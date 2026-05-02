// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testaerodynamics.cpp
 * @brief aerodynamics tests.
 */

#include "testaerodynamics.hpp"

#include <koilo/systems/aerodynamics/aero_module.hpp>
#include <koilo/systems/aerodynamics/aerocurve.hpp>
#include <koilo/systems/aerodynamics/aerodynamicsurface.hpp>
#include <koilo/systems/aerodynamics/aerodynamicsworld.hpp>
#include <koilo/systems/aerodynamics/atmosphere.hpp>
#include <koilo/systems/aerodynamics/thrustengine.hpp>
#include <koilo/systems/aerodynamics/windfield.hpp>

#include <koilo/systems/physics/physicsworld.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/bodypose.hpp>

#include <utils/testhelpers.hpp>

#include <cmath>
#include <cstring>
#include <vector>

using namespace koilo;
using namespace koilo::aero;

// ===========================================================================
// Atmosphere
// ===========================================================================

void TestAerodynamics::TestAtmosphereSeaLevel() {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 288.15f, isa::Temperature(0.0f));
    TEST_ASSERT_FLOAT_WITHIN(1.0f,   101325.0f, isa::Pressure(0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.225f,  isa::Density(0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.5f,   340.29f, isa::SpeedOfSound(0.0f));
}

void TestAerodynamics::TestAtmospherePressureMonotonic() {
    float prev = isa::Pressure(0.0f);
    for (float h = 500.0f; h <= 19500.0f; h += 500.0f) {
        const float p = isa::Pressure(h);
        TEST_ASSERT_TRUE(p < prev);
        prev = p;
    }
}

void TestAerodynamics::TestAtmosphereTropopauseContinuity() {
    // At the layer boundary the two formulas should agree.
    const float pBelow = isa::Pressure(11000.0f - 0.001f);
    const float pAt    = isa::Pressure(11000.0f);
    const float pAbove = isa::Pressure(11000.0f + 0.001f);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, pBelow, pAt);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, pAt,    pAbove);
}

void TestAerodynamics::TestAtmosphereDensityAt11km() {
    // ISA reference density at 11 km ~= 0.3639 kg/m^3.
    TEST_ASSERT_FLOAT_WITHIN(0.005f, 0.3639f, isa::Density(11000.0f));
}

void TestAerodynamics::TestAtmosphereClampsAltitude() {
    // Negative altitude clamps to sea level.
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.225f, isa::Density(-100.0f));
    // Above kHMax clamps to kHMax.
    const float dTop = isa::Density(isa::kHMax);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, dTop, isa::Density(isa::kHMax + 5000.0f));
}

// ===========================================================================
// AeroCurve
// ===========================================================================

void TestAerodynamics::TestAeroCurveEmptyReturnsZero() {
    AeroCurve c;
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 0.0f, c.Sample(0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 0.0f, c.Sample(1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 0.0f, c.Sample(-1.0f));
}

void TestAerodynamics::TestAeroCurveSinglePointReturnsValue() {
    AeroCurve c({0.1f}, {7.5f});
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 7.5f, c.Sample(-10.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 7.5f, c.Sample(0.1f));
    TEST_ASSERT_FLOAT_WITHIN(0.0f, 7.5f, c.Sample(99.0f));
}

void TestAerodynamics::TestAeroCurveMidpointLerp() {
    AeroCurve c({0.0f, 1.0f}, {0.0f, 10.0f});
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f,  c.Sample(0.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.5f,  c.Sample(0.25f));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 8.0f,  c.Sample(0.8f));
}

void TestAerodynamics::TestAeroCurveClampsEndpoints() {
    AeroCurve c({-0.5f, 0.0f, 0.5f}, {-1.0f, 0.0f, 2.0f});
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -1.0f, c.Sample(-100.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f,  2.0f, c.Sample( 100.0f));
}

// ===========================================================================
// WindField
// ===========================================================================

void TestAerodynamics::TestConstantWindReturnsConstant() {
    ConstantWind w(Vector3D(3.0f, -1.0f, 2.0f));
    const Vector3D v = w.Sample(Vector3D(100.0f, 200.0f, -50.0f), 9.0f);
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(3.0f, -1.0f, 2.0f), v);
}

void TestAerodynamics::TestShearWindLinearWithAltitude() {
    ShearWind w(Vector3D(1.0f, 0.0f, 0.0f), Vector3D(0.5f, 0.0f, 0.0f));
    // At Y=0: base only.
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(1.0f, 0.0f, 0.0f),
        w.Sample(Vector3D(10.0f, 0.0f, 7.0f), 0.0f));
    // At Y=10: 1 + 0.5*10 = 6.
    TEST_ASSERT_VECTOR3D_EQUAL(Vector3D(6.0f, 0.0f, 0.0f),
        w.Sample(Vector3D(0.0f, 10.0f, 0.0f), 0.0f));
}

// ===========================================================================
// AerodynamicSurface compute
// ===========================================================================

namespace {
RigidBody MakeFlyingBody(float mass = 1.0f) {
    RigidBody b(BodyType::Dynamic, mass);
    b.SetInertiaSphere(0.5f);
    b.SetLinearDamping(0.0f);
    b.SetAngularDamping(0.0f);
    return b;
}

AerodynamicSurface MakeWing() {
    AerodynamicSurface s;
    s.centerOfPressureLocal = Vector3D(0, 0, 0);
    s.forwardLocal = Vector3D(1, 0, 0);
    s.upLocal      = Vector3D(0, 1, 0);
    s.area         = 1.0f;
    s.controlDeflectionRad = 0.0f;
    s.cl = AeroCurve::MakeFlatPlateLift();
    s.cd = AeroCurve::MakeFlatPlateDrag();
    return s;
}
} // namespace

void TestAerodynamics::TestSurfaceZeroAirflowZeroForce() {
    PhysicsWorld pw(Vector3D(0,0,0));
    auto body = MakeFlyingBody();
    body.SetPose(BodyPose(Vector3D(0, 1000.0f, 0)));
    pw.AddBody(&body);

    AerodynamicsWorld aw;
    aw.AttachToPhysics(&pw);
    auto wing = MakeWing();
    aw.RegisterSurface(&body, &wing);

    pw.Step(pw.GetFixedTimestep());
    // Body had zero velocity and no wind set: no aero force, only gravity (0).
    TEST_ASSERT_VECTOR3D_WITHIN(1e-4f, Vector3D(0,0,0), body.GetVelocity());
}

void TestAerodynamics::TestSurfaceLevelFlowZeroAoaDragOnly() {
    // Body moving forward (+X) into still air -> AoA = 0 -> CL=0, CD=CD0=0.02.
    // Force should be purely along -X (drag opposing motion), no Y component.
    PhysicsWorld pw(Vector3D(0,0,0));
    auto body = MakeFlyingBody();
    body.SetPose(BodyPose(Vector3D(0, 0, 0)));
    body.SetVelocity(Vector3D(20.0f, 0.0f, 0.0f));
    pw.AddBody(&body);

    AerodynamicsWorld aw;
    aw.AttachToPhysics(&pw);
    auto wing = MakeWing();
    aw.RegisterSurface(&body, &wing);

    const Vector3D v0 = body.GetVelocity();
    pw.Step(pw.GetFixedTimestep());
    const Vector3D v1 = body.GetVelocity();
    const Vector3D dv = v1 - v0;
    // X velocity decreased (drag). Y essentially unchanged.
    TEST_ASSERT_TRUE(dv.X < 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, dv.Y);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, dv.Z);
}

void TestAerodynamics::TestSurfacePositiveAoaPositiveLift() {
    // Body moving forward but pitched up by ~15 deg: relative airflow comes
    // from forward+below, AoA is positive, lift should push body up (+Y).
    PhysicsWorld pw(Vector3D(0,0,0));
    auto body = MakeFlyingBody();
    // Pitch up around Z axis by +15 deg using axis-angle quaternion.
    const float pitchRad = 0.26f;
    Quaternion q;
    {
        const float h = 0.5f * pitchRad;
        const float s = std::sin(h);
        // rotation about world +Z (right-hand rule): pitches +X up.
        q = Quaternion(std::cos(h), 0.0f, 0.0f, s);
    }
    body.SetPose(BodyPose(Vector3D(0,0,0), q));
    body.SetVelocity(Vector3D(20.0f, 0.0f, 0.0f));
    pw.AddBody(&body);

    AerodynamicsWorld aw;
    aw.AttachToPhysics(&pw);
    auto wing = MakeWing();
    aw.RegisterSurface(&body, &wing);

    pw.Step(pw.GetFixedTimestep());
    // After one substep, vertical velocity should be > 0 (lift).
    TEST_ASSERT_TRUE(body.GetVelocity().Y > 0.0f);
}

void TestAerodynamics::TestSurfacePureSideslipNearZeroForce() {
    // Body moving along +Z (spanwise), wing pointing forward along +X.
    // Chord plane is X-Y; sideslip Z component should be projected out.
    PhysicsWorld pw(Vector3D(0,0,0));
    auto body = MakeFlyingBody();
    body.SetVelocity(Vector3D(0.0f, 0.0f, 20.0f));
    pw.AddBody(&body);

    AerodynamicsWorld aw;
    aw.AttachToPhysics(&pw);
    auto wing = MakeWing();
    aw.RegisterSurface(&body, &wing);

    const Vector3D v0 = body.GetVelocity();
    pw.Step(pw.GetFixedTimestep());
    const Vector3D v1 = body.GetVelocity();
    // Pure spanwise flow projected out -> chord-plane speed below epsilon
    // -> compute exits early -> zero force.
    TEST_ASSERT_VECTOR3D_WITHIN(1e-5f, v0, v1);
}

void TestAerodynamics::TestSurfaceForceAppliesLeverArmTorque() {
    // COP offset from origin -> off-COM force should induce torque, picked
    // up as angular velocity change.
    PhysicsWorld pw(Vector3D(0,0,0));
    auto body = MakeFlyingBody();
    body.SetVelocity(Vector3D(20.0f, 0.0f, 0.0f));
    pw.AddBody(&body);

    AerodynamicsWorld aw;
    aw.AttachToPhysics(&pw);
    auto wing = MakeWing();
    wing.centerOfPressureLocal = Vector3D(0.0f, 0.0f, 1.0f); // 1 m to the side
    // Pitch up so AoA produces lift.
    aw.RegisterSurface(&body, &wing);
    const float pitchRad = 0.26f;
    Quaternion q(std::cos(0.5f*pitchRad), 0.0f, 0.0f, std::sin(0.5f*pitchRad));
    body.SetPose(BodyPose(Vector3D(0,0,0), q));

    const Vector3D w0 = body.GetAngularVelocity();
    pw.Step(pw.GetFixedTimestep());
    const Vector3D w1 = body.GetAngularVelocity();
    // Some component of angular velocity must have changed.
    const Vector3D dw = w1 - w0;
    TEST_ASSERT_TRUE(dw.Magnitude() > 1e-6f);
}

// ===========================================================================
// ThrustEngine
// ===========================================================================

void TestAerodynamics::TestThrustZeroThrottleZeroForce() {
    PhysicsWorld pw(Vector3D(0,0,0));
    auto body = MakeFlyingBody();
    pw.AddBody(&body);
    AerodynamicsWorld aw;
    aw.AttachToPhysics(&pw);
    ThrustEngine eng;
    eng.directionLocal = Vector3D(1,0,0);
    eng.maxThrustN = 100.0f;
    eng.throttle = 0.0f;
    aw.RegisterEngine(&body, &eng);

    pw.Step(pw.GetFixedTimestep());
    TEST_ASSERT_VECTOR3D_WITHIN(1e-5f, Vector3D(0,0,0), body.GetVelocity());
}

void TestAerodynamics::TestThrustFullThrottleAccelerates() {
    PhysicsWorld pw(Vector3D(0,0,0));
    auto body = MakeFlyingBody(2.0f); // mass 2 kg
    pw.AddBody(&body);
    AerodynamicsWorld aw;
    aw.AttachToPhysics(&pw);
    ThrustEngine eng;
    eng.directionLocal = Vector3D(1,0,0);
    eng.maxThrustN = 100.0f;
    eng.throttle = 1.0f;
    aw.RegisterEngine(&body, &eng);

    const float dt = pw.GetFixedTimestep();
    pw.Step(dt);
    // a = F/m = 100 / 2 = 50 m/s^2. dv = 50 * dt.
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 50.0f * dt, body.GetVelocity().X);
}

void TestAerodynamics::TestThrustFuelBurnDecreasesMass() {
    PhysicsWorld pw(Vector3D(0,0,0));
    auto body = MakeFlyingBody(11.0f);  // dry 10 + fuel 1
    pw.AddBody(&body);
    AerodynamicsWorld aw;
    aw.AttachToPhysics(&pw);
    ThrustEngine eng;
    eng.directionLocal = Vector3D(1,0,0);
    eng.maxThrustN = 200.0f;
    eng.throttle = 1.0f;
    eng.specificImpulseSec = 250.0f;
    eng.fuelMassKg = 1.0f;
    eng.dryMassKg  = 10.0f;
    aw.RegisterEngine(&body, &eng);

    const float dt = pw.GetFixedTimestep();
    // 10 substeps. mdot = 200/(250*9.80665) ~= 0.08157 kg/s.
    const float expectedBurn = 0.08157f * dt * 10.0f;
    for (int i = 0; i < 10; ++i) pw.Step(dt);

    const float remainingFuel = eng.fuelMassKg;
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 1.0f - expectedBurn, remainingFuel);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 10.0f + remainingFuel, body.GetMass());
}

void TestAerodynamics::TestThrustFuelExhaustionStopsThrust() {
    PhysicsWorld pw(Vector3D(0,0,0));
    auto body = MakeFlyingBody(10.001f);
    pw.AddBody(&body);
    AerodynamicsWorld aw;
    aw.AttachToPhysics(&pw);
    ThrustEngine eng;
    eng.directionLocal = Vector3D(1,0,0);
    eng.maxThrustN = 100000.0f;       // huge -> burn out fast
    eng.throttle = 1.0f;
    eng.specificImpulseSec = 250.0f;
    eng.fuelMassKg = 0.001f;          // tiny
    eng.dryMassKg  = 10.0f;
    aw.RegisterEngine(&body, &eng);

    // Run plenty of substeps to ensure fuel runs out.
    const float dt = pw.GetFixedTimestep();
    for (int i = 0; i < 200; ++i) pw.Step(dt);

    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, eng.fuelMassKg);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 10.0f, body.GetMass());
    // Capture velocity, run 5 more steps; with no fuel the throttle is gated
    // to zero so velocity should not change.
    const Vector3D v_before = body.GetVelocity();
    for (int i = 0; i < 5; ++i) pw.Step(dt);
    TEST_ASSERT_VECTOR3D_WITHIN(1e-4f, v_before, body.GetVelocity());
}

// ===========================================================================
// World + lifecycle
// ===========================================================================

void TestAerodynamics::TestWorldRegisterUnregister() {
    AerodynamicsWorld aw;
    RigidBody body(BodyType::Dynamic, 1.0f);
    AerodynamicSurface s;
    auto id1 = aw.RegisterSurface(&body, &s);
    auto id2 = aw.RegisterSurface(&body, &s);
    TEST_ASSERT_TRUE(id1 != AerodynamicsWorld::kInvalidId);
    TEST_ASSERT_TRUE(id2 != AerodynamicsWorld::kInvalidId);
    TEST_ASSERT_TRUE(id1 != id2);
    TEST_ASSERT_EQUAL_UINT(2, aw.GetSurfaceCount());
    aw.UnregisterSurface(id1);
    TEST_ASSERT_EQUAL_UINT(1, aw.GetSurfaceCount());
    aw.UnregisterSurface(id1); // double-unregister no-op
    TEST_ASSERT_EQUAL_UINT(1, aw.GetSurfaceCount());
    aw.Clear();
    TEST_ASSERT_EQUAL_UINT(0, aw.GetSurfaceCount());

    // Null params return invalid id.
    TEST_ASSERT_EQUAL_UINT(AerodynamicsWorld::kInvalidId,
        aw.RegisterSurface(nullptr, &s));
    TEST_ASSERT_EQUAL_UINT(AerodynamicsWorld::kInvalidId,
        aw.RegisterSurface(&body, nullptr));
}

void TestAerodynamics::TestPreSubstepHookFiresPerSubstep() {
    // Verify the PhysicsWorld pre-fixed-step callback fires once per substep.
    PhysicsWorld pw(Vector3D(0,0,0));
    int counter = 0;
    pw.RegisterPreFixedStepCallback([&counter](float){ ++counter; });

    const float dt = pw.GetFixedTimestep();
    pw.Step(dt);             // 1 substep
    pw.Step(dt * 3.0f);      // 3 substeps
    TEST_ASSERT_EQUAL_INT(4, counter);
}

void TestAerodynamics::TestModuleAttachToPhysics() {
    // Smoke test: AerodynamicsWorld can attach/detach without crashing,
    // and Step() forwards through the hook even with no surfaces.
    PhysicsWorld pw(Vector3D(0,0,0));
    {
        AerodynamicsWorld aw;
        aw.AttachToPhysics(&pw);
        pw.Step(pw.GetFixedTimestep());
        // Detaching should clear callbacks; second Step does nothing.
        aw.DetachFromPhysics();
        pw.Step(pw.GetFixedTimestep());
    }
    // After aero world destroyed, physics still steps.
    pw.Step(pw.GetFixedTimestep());
    TEST_ASSERT_TRUE(true);
}

// ===========================================================================
// wrap-up - keyed unregister + wake-on-thrust
// ===========================================================================

void TestAerodynamics::TestPreFixedStepKeyedUnregister() {
    // Register two callbacks, unregister only the first, and verify the
    // second still fires.
    PhysicsWorld pw(Vector3D(0,0,0));
    int countA = 0, countB = 0;
    auto idA = pw.RegisterPreFixedStepCallback([&](float){ ++countA; });
    auto idB = pw.RegisterPreFixedStepCallback([&](float){ ++countB; });
    TEST_ASSERT_TRUE(idA != kInvalidPreFixedStepCallbackId);
    TEST_ASSERT_TRUE(idB != kInvalidPreFixedStepCallbackId);
    TEST_ASSERT_TRUE(idA != idB);

    const float dt = pw.GetFixedTimestep();
    pw.Step(dt);
    TEST_ASSERT_EQUAL_INT(1, countA);
    TEST_ASSERT_EQUAL_INT(1, countB);

    pw.UnregisterPreFixedStepCallback(idA);
    pw.Step(dt);
    TEST_ASSERT_EQUAL_INT(1, countA);   // still 1 - A removed
    TEST_ASSERT_EQUAL_INT(2, countB);   // B kept ticking

    // Unknown / invalid id is a safe no-op.
    pw.UnregisterPreFixedStepCallback(idA);
    pw.UnregisterPreFixedStepCallback(kInvalidPreFixedStepCallbackId);
    pw.Step(dt);
    TEST_ASSERT_EQUAL_INT(1, countA);
    TEST_ASSERT_EQUAL_INT(3, countB);
}

void TestAerodynamics::TestPreFixedStepClearStillWorks() {
    // Sledgehammer Clear still removes everything (back-compat for tests).
    PhysicsWorld pw(Vector3D(0,0,0));
    int count = 0;
    pw.RegisterPreFixedStepCallback([&](float){ ++count; });
    pw.RegisterPreFixedStepCallback([&](float){ ++count; });
    pw.ClearPreFixedStepCallbacks();
    pw.Step(pw.GetFixedTimestep());
    TEST_ASSERT_EQUAL_INT(0, count);
}

void TestAerodynamics::TestEngineWakesSleepingBody() {
    // a sleeping body must wake when engine thrust is non-zero.
    // Without this, ApplyForceAtPoint queues the force but IntegrateVelocity
    // skips sleeping bodies - so the engine impulse silently vanishes.
    PhysicsWorld pw(Vector3D(0,0,0)); // no gravity -> easier to assert
    AerodynamicsWorld aw;
    aw.AttachToPhysics(&pw);

    RigidBody body(BodyType::Dynamic, 1.0f);
    pw.AddBody(&body);

    ThrustEngine eng;
    eng.directionLocal = Vector3D(1,0,0);
    eng.maxThrustN = 100.0f;
    eng.throttle   = 1.0f;
    aw.RegisterEngine(&body, &eng);

    body.Sleep();
    TEST_ASSERT_TRUE(body.IsSleeping());

    pw.Step(pw.GetFixedTimestep());

    TEST_ASSERT_FALSE(body.IsSleeping());
    TEST_ASSERT_TRUE(body.GetVelocity().X > 0.0f);

    aw.DetachFromPhysics();
}

// ===========================================================================
// Determinism - same-binary replay
// ===========================================================================

namespace {
struct PoseSnap { Vector3D p; Quaternion q; Vector3D v; Vector3D w; };

std::vector<PoseSnap> RunFlightScene(int steps) {
    PhysicsWorld pw(Vector3D(0, -9.81f, 0));
    pw.SetDeterminismTier(DeterminismTier::T1_SameBinaryReplay);

    RigidBody body(BodyType::Dynamic, 5.0f);
    body.SetInertiaSphere(0.5f);
    body.SetLinearDamping(0.0f);
    body.SetAngularDamping(0.0f);
    body.SetPose(BodyPose(Vector3D(0, 100.0f, 0)));
    body.SetVelocity(Vector3D(40.0f, 0.0f, 0.0f));
    pw.AddBody(&body);

    AerodynamicsWorld aw;
    aw.AttachToPhysics(&pw);
    AerodynamicSurface wing;
    wing.centerOfPressureLocal = Vector3D(0, 0, 0);
    wing.forwardLocal = Vector3D(1, 0, 0);
    wing.upLocal      = Vector3D(0, 1, 0);
    wing.area         = 5.0f;
    wing.cl = AeroCurve::MakeFlatPlateLift();
    wing.cd = AeroCurve::MakeFlatPlateDrag();
    aw.RegisterSurface(&body, &wing);

    ThrustEngine eng;
    eng.directionLocal = Vector3D(1, 0, 0);
    eng.maxThrustN = 200.0f;
    eng.throttle = 0.5f;
    aw.RegisterEngine(&body, &eng);

    const float dt = pw.GetFixedTimestep();
    std::vector<PoseSnap> out;
    out.reserve(steps);
    for (int i = 0; i < steps; ++i) {
        pw.Step(dt);
        out.push_back({
            body.GetPose().position,
            body.GetPose().orientation,
            body.GetVelocity(),
            body.GetAngularVelocity(),
        });
    }
    return out;
}
} // namespace

void TestAerodynamics::TestSameBinaryReplayBitExact() {
    auto a = RunFlightScene(120);
    auto b = RunFlightScene(120);
    TEST_ASSERT_EQUAL_UINT(a.size(), b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        TEST_ASSERT_EQUAL_INT(0, std::memcmp(&a[i], &b[i], sizeof(PoseSnap)));
    }
}

// ===========================================================================
// Runner
// ===========================================================================

void TestAerodynamics::RunAllTests() {
    RUN_TEST(TestAtmosphereSeaLevel);
    RUN_TEST(TestAtmospherePressureMonotonic);
    RUN_TEST(TestAtmosphereTropopauseContinuity);
    RUN_TEST(TestAtmosphereDensityAt11km);
    RUN_TEST(TestAtmosphereClampsAltitude);

    RUN_TEST(TestAeroCurveEmptyReturnsZero);
    RUN_TEST(TestAeroCurveSinglePointReturnsValue);
    RUN_TEST(TestAeroCurveMidpointLerp);
    RUN_TEST(TestAeroCurveClampsEndpoints);

    RUN_TEST(TestConstantWindReturnsConstant);
    RUN_TEST(TestShearWindLinearWithAltitude);

    RUN_TEST(TestSurfaceZeroAirflowZeroForce);
    RUN_TEST(TestSurfaceLevelFlowZeroAoaDragOnly);
    RUN_TEST(TestSurfacePositiveAoaPositiveLift);
    RUN_TEST(TestSurfacePureSideslipNearZeroForce);
    RUN_TEST(TestSurfaceForceAppliesLeverArmTorque);

    RUN_TEST(TestThrustZeroThrottleZeroForce);
    RUN_TEST(TestThrustFullThrottleAccelerates);
    RUN_TEST(TestThrustFuelBurnDecreasesMass);
    RUN_TEST(TestThrustFuelExhaustionStopsThrust);

    RUN_TEST(TestWorldRegisterUnregister);
    RUN_TEST(TestPreSubstepHookFiresPerSubstep);
    RUN_TEST(TestModuleAttachToPhysics);

    RUN_TEST(TestPreFixedStepKeyedUnregister);
    RUN_TEST(TestPreFixedStepClearStillWorks);
    RUN_TEST(TestEngineWakesSleepingBody);

    RUN_TEST(TestSameBinaryReplayBitExact);
}
