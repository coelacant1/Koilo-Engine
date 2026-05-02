// SPDX-License-Identifier: GPL-3.0-or-later
#include "aerodynamicsworld.hpp"

#include "atmosphere.hpp"

#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/bodypose.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/quaternion.hpp>

#include <cmath>

namespace koilo::aero {

// Standard gravity used for Isp -> mdot. Per convention this is g0 = 9.80665
// m/s^2 NOT the local gravity vector - that is how rocket-engine specific
// impulse is defined. See kIsa::kG0.
static constexpr float kIspG0 = 9.80665f;

// Minimum chord-plane airspeed [m/s] below which we skip aero force
// computation entirely. Prevents division by zero in the lift / drag
// direction normalization without introducing a smoothed-but-incorrect
// non-zero force at near-zero airspeed. Set above the typical sleep
// velocity threshold (~0.2 m/s for unit-mass bodies) so a body slowing
// down can drop into the sleep window without aero perpetually waking it.
static constexpr float kMinChordPlaneSpeed = 5.0e-2f;

AerodynamicsWorld::AerodynamicsWorld() = default;

AerodynamicsWorld::~AerodynamicsWorld() {
    DetachFromPhysics();
}

void AerodynamicsWorld::AttachToPhysics(PhysicsWorld* world) {
    if (!world || attached_) return;
    physics_ = world;
    preStepCallbackId_ = physics_->RegisterPreFixedStepCallback(
        [this](float dt) { this->Step(dt); });
    attached_ = true;
}

void AerodynamicsWorld::DetachFromPhysics() {
    if (!attached_ || !physics_) {
        physics_ = nullptr;
        attached_ = false;
        preStepCallbackId_ = 0;
        return;
    }
    if (preStepCallbackId_ != 0) {
        physics_->UnregisterPreFixedStepCallback(preStepCallbackId_);
        preStepCallbackId_ = 0;
    }
    physics_ = nullptr;
    attached_ = false;
}

AerodynamicsWorld::SurfaceId AerodynamicsWorld::RegisterSurface(
    RigidBody* body, AerodynamicSurface* surface)
{
    if (!body || !surface) return kInvalidId;
    SurfaceId id = nextId_++;
    surfaces_.push_back({id, body, surface});
    return id;
}

AerodynamicsWorld::EngineId AerodynamicsWorld::RegisterEngine(
    RigidBody* body, ThrustEngine* engine)
{
    if (!body || !engine) return kInvalidId;
    EngineId id = nextId_++;
    engines_.push_back({id, body, engine});
    return id;
}

void AerodynamicsWorld::UnregisterSurface(SurfaceId id) {
    if (id == kInvalidId) return;
    for (auto it = surfaces_.begin(); it != surfaces_.end(); ++it) {
        if (it->id == id) { surfaces_.erase(it); return; }
    }
}

void AerodynamicsWorld::UnregisterEngine(EngineId id) {
    if (id == kInvalidId) return;
    for (auto it = engines_.begin(); it != engines_.end(); ++it) {
        if (it->id == id) { engines_.erase(it); return; }
    }
}

void AerodynamicsWorld::Clear() {
    surfaces_.clear();
    engines_.clear();
}

void AerodynamicsWorld::Step(float fixedDt) {
    if (fixedDt <= 0.0f) return;
    // Engines run first so any mass changes are visible to whatever uses
    // the body next; aero surfaces don't read body mass.
    for (auto& e : engines_)  { ApplyEngine(e, fixedDt);  }
    for (auto& s : surfaces_) { ApplySurface(s, fixedDt); }
    simTime_ += fixedDt;
}

// --- helpers --------------------------------------------------------------

static inline float SafeNormalizeOrZero(Vector3D& v) {
    const float m = v.Magnitude();
    if (m <= 0.0f) { v = Vector3D(0.0f, 0.0f, 0.0f); return 0.0f; }
    v = v * (1.0f / m);
    return m;
}

static inline Vector3D RotateLocalToWorld(const Quaternion& q, const Vector3D& v) {
    return q.RotateVector(v);
}

// --- per-surface compute --------------------------------------------------

void AerodynamicsWorld::ApplySurface(const SurfaceEntry& entry, float fixedDt) {
    (void)fixedDt;
    RigidBody* body = entry.body;
    AerodynamicSurface* s = entry.surface;
    if (!body || !s || !body->IsDynamic()) {
        if (s) s->lastForceWorld = Vector3D(0.0f, 0.0f, 0.0f);
        return;
    }
    // Skip sleeping bodies: a sleeping body has zero (clamped) velocity,
    // so airflow at any COP is zero -> force is zero. Calling
    // ApplyForceAtPoint with a tiny noise force would Wake() the body
    // every step and prevent the rocket from ever settling.
    if (body->IsSleeping()) {
        s->lastForceWorld = Vector3D(0.0f, 0.0f, 0.0f);
        return;
    }

    const BodyPose pose = body->GetPose();

    // 1. Build a robust orthonormal chord frame (forward, up, right).
    //    forwardLocal MUST be non-zero. upLocal is reprojected to be
    //    perpendicular to forwardLocal so authored values can be sloppy.
    Vector3D fwdLocal = s->forwardLocal;
    if (SafeNormalizeOrZero(fwdLocal) <= 0.0f) return;
    Vector3D upLocal = s->upLocal;
    // Reproject: up' = up - (up . fwd) * fwd, then normalize. Falls back
    // to a synthetic axis if degenerate.
    {
        const float d = upLocal.DotProduct(fwdLocal);
        upLocal = upLocal - fwdLocal * d;
        if (SafeNormalizeOrZero(upLocal) <= 0.0f) {
            // Pick any axis not parallel to fwdLocal.
            Vector3D candidate = (std::fabs(fwdLocal.Y) < 0.9f)
                ? Vector3D(0.0f, 1.0f, 0.0f)
                : Vector3D(1.0f, 0.0f, 0.0f);
            upLocal = candidate - fwdLocal * candidate.DotProduct(fwdLocal);
            if (SafeNormalizeOrZero(upLocal) <= 0.0f) return;
        }
    }
    // span axis (right-handed): right = forward x up
    Vector3D rightLocal = fwdLocal.CrossProduct(upLocal);
    SafeNormalizeOrZero(rightLocal);

    // To world.
    const Vector3D fwdWorld   = RotateLocalToWorld(pose.orientation, fwdLocal);
    const Vector3D upWorld    = RotateLocalToWorld(pose.orientation, upLocal);
    const Vector3D rightWorld = RotateLocalToWorld(pose.orientation, rightLocal);
    const Vector3D copWorld   = pose.position + RotateLocalToWorld(pose.orientation, s->centerOfPressureLocal);

    // 2. Velocity at the COP: v_lin + omega x r.
    const Vector3D r = copWorld - pose.position;
    const Vector3D vAtCop = body->GetVelocity()
        + body->GetAngularVelocity().CrossProduct(r);

    // 3. Sample wind and form relative airflow (vRel = wind - vBody).
    const Vector3D wind = (wind_ != nullptr)
        ? wind_->Sample(copWorld, simTime_)
        : Vector3D(0.0f, 0.0f, 0.0f);
    const Vector3D vRel = wind - vAtCop;

    // 4. Project vRel onto the chord plane (drop the spanwise component).
    // Sideslip is treated as ignored v0 scope: both the
    //    AoA and the dynamic pressure use the in-plane velocity.
    const float vSpan = vRel.DotProduct(rightWorld);
    const Vector3D vChord = vRel - rightWorld * vSpan;
    const float chordSpeed = vChord.Magnitude();
    if (chordSpeed < kMinChordPlaneSpeed) return;

    // 5. AoA from the relative air velocity vector v_a = -vRel (the velocity
    //    of the body through the air, equivalent to the freestream as seen
    //    from the body). Positive AoA means the chord is pitched up relative
    //    to v_a, i.e. the air arrives from forward-and-below the chord.
    //    With vChord = vRel projected to chord plane:
    //      v_a · forward = -vChord · forward = -vFwdComp
    //      v_a · up      = -vChord · up      = -vUpComp
    //    alpha = atan2(-(v_a · up), v_a · forward)
    //          = atan2(vUpComp, -vFwdComp)
    const float vFwdComp = vChord.DotProduct(fwdWorld);
    const float vUpComp  = vChord.DotProduct(upWorld);
    float aoa = std::atan2(vUpComp, -vFwdComp) + s->controlDeflectionRad;

    // Fold AoA into [-pi/2, pi/2]: a flat plate flying backwards (|aoa|>pi/2)
    // is physically equivalent to one at the mirrored AoA with the lift
    // direction reversed (the wing's leading and trailing edges swap, but the
    // plate's "up" face is now the lower face relative to the freestream).
    // Without this fold the curve clamps to its edge value at +/- pi rad and
    // generates phantom lift when a body falls or climbs along its forward
    // axis (e.g. a rocket falling tail-first sees aoa = pi at every fin).
    constexpr float kHalfPi = 1.5707963267948966f;
    constexpr float kPi     = 3.1415926535897932f;
    // Wrap to (-pi, pi]
    while (aoa >  kPi) aoa -= 2.0f * kPi;
    while (aoa < -kPi) aoa += 2.0f * kPi;
    bool reversed = false;
    if (aoa > kHalfPi)       { aoa = kPi  - aoa; reversed = true; }
    else if (aoa < -kHalfPi) { aoa = -kPi - aoa; reversed = true; }

    // 6. Atmosphere lookup (altitude = pose.position . worldUp).
    const float altitude = pose.position.DotProduct(worldUp_);
    const float rho = isa::Density(altitude);
    const float q   = 0.5f * rho * chordSpeed * chordSpeed;

    // 7. Coefficients.
    float CL = s->cl.Sample(aoa);
    const float CD = s->cd.Sample(aoa);
    if (reversed) CL = -CL;

    // 8. Drag direction = vChord/|vChord| (= vRel direction = -v_a direction).
    //    Drag opposes the body's motion through the air: F_drag is along vRel.
    Vector3D dragDir = vChord * (1.0f / chordSpeed);
    // Lift direction is perpendicular to drag in the chord plane and oriented
    // so that positive AoA produces a force component along upWorld for the
    // canonical orientation (forward = chord, up = chord-normal):
    //   lift = drag x right
    Vector3D liftDir = dragDir.CrossProduct(rightWorld);
    if (SafeNormalizeOrZero(liftDir) <= 0.0f) return;

    const Vector3D force = (liftDir * (q * s->area * CL))
                         + (dragDir * (q * s->area * CD));

    body->ApplyForceAtPoint(force, copWorld);

    // Debug snapshot for visualization (read by scripts/HUD).
    s->lastForceWorld = force;
    s->lastCopWorld   = copWorld;
    s->lastAoaRad     = aoa;
    s->lastDynPress   = q;
}

// --- per-engine compute ---------------------------------------------------

void AerodynamicsWorld::ApplyEngine(const EngineEntry& entry, float fixedDt) {
    RigidBody* body = entry.body;
    ThrustEngine* eng = entry.engine;
    if (!body || !eng || !body->IsDynamic()) return;

    float throttle = eng->throttle;
    if (throttle < 0.0f) throttle = 0.0f;
    if (throttle > 1.0f) throttle = 1.0f;

    // Cut off thrust if fuel is tracked and exhausted.
    if (eng->specificImpulseSec > 0.0f && eng->fuelMassKg <= 0.0f) {
        throttle = 0.0f;
    }
    const float thrustN = throttle * eng->maxThrustN;
    if (thrustN <= 0.0f) return;

    // wake the body before applying thrust. ApplyForceAtPoint
    // queues a force on the accumulator, but IntegrateVelocity is a no-op for
    // sleeping bodies - so without an explicit Wake() the engine impulse
    // would silently vanish frame after frame. Surfaces don't need this:
    // a sleeping body has zero velocity -> zero airflow -> zero force, so the
    // sleep state is self-consistent for aero surfaces.
    if (body->IsSleeping()) {
        body->Wake();
    }

    // Direction (normalized) and application point in world.
    Vector3D dirLocal = eng->directionLocal;
    if (SafeNormalizeOrZero(dirLocal) <= 0.0f) return;
    const BodyPose pose = body->GetPose();
    const Vector3D dirWorld = pose.orientation.RotateVector(dirLocal);
    const Vector3D appWorld = pose.position
        + pose.orientation.RotateVector(eng->applicationPointLocal);

    body->ApplyForceAtPoint(dirWorld * thrustN, appWorld);

    // Fuel burn (after force application). mdot = T / (Isp * g0).
    if (eng->specificImpulseSec > 0.0f) {
        const float mdot = thrustN / (eng->specificImpulseSec * kIspG0);
        eng->fuelMassKg -= mdot * fixedDt;
        if (eng->fuelMassKg < 0.0f) eng->fuelMassKg = 0.0f;
        // Update body mass = dry + remaining fuel.
        body->SetMass(eng->dryMassKg + eng->fuelMassKg);
    }
}

} // namespace koilo::aero
