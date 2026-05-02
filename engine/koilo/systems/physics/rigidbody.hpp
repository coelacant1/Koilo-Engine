// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file rigidbody.hpp
 * @brief RigidBody component for physics simulation.
 *
 * Stores velocity, mass, forces, and integrates motion.
 * Attaches to a Collider for collision response.
 *
 * @date 21/02/2025
 * @author Coela Can't
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/matrix3x3.hpp>
#include "bodypose.hpp"
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

class Collider;

/**
 * @enum BodyType
 * @brief Determines how a rigid body participates in physics.
 */
enum class BodyType {
    Static,     ///< Never moves (infinite mass, zero velocity)
    Kinematic,  ///< Moved by user code, not by forces (infinite mass for collision response)
    Dynamic     ///< Fully simulated (forces, gravity, collision impulses)
};

/**
 * @class RigidBody
 * @brief Physics body with mass, velocity, and force accumulation.
 */
class RigidBody {
private:
    BodyType bodyType_;
    float mass_;
    float inverseMass_;
    float restitution_;     ///< Bounciness (0 = no bounce, 1 = perfect bounce)
    float friction_;        ///< Friction coefficient
    float linearDamping_;   ///< Velocity damping per second (0 = none, 1 = full stop)

    Vector3D velocity_;
    Vector3D forceAccumulator_;
    Vector3D angularVelocity_;       ///< World-space angular velocity (rad/s).
    Vector3D torqueAccumulator_;     ///< World-space torque accumulator (N·m).
    Matrix3x3 inertiaLocal_;         ///< Inertia tensor in local frame; zero -> angular response disabled.
    Matrix3x3 invInertiaLocal_;      ///< Inverse of inertiaLocal_ (zero rows are kept zero).
    Matrix3x3 invInertiaWorld_;      ///< Refreshed each Integrate from orientation.
    float angularDamping_;           ///< Angular damping per second.
    Collider* collider_;    ///< Associated collider (non-owning)

    // === Sleeping ===
    bool  sleeping_      = false;     ///< When true, integrator + solver treat the body as static.
    bool  allowSleep_    = true;      ///< If false, body is never put to sleep (e.g. player-controlled).
    float sleepTimer_    = 0.0f;      ///< Seconds of continuous below-threshold motion accumulated.

    // === CCD ===
    bool  bullet_        = false;     ///< When true, broadphase swept margin + speculative contacts apply.

    BodyPose pose_;          ///< Current world pose.
    BodyPose previousPose_;  ///< Pose at start of last fixed substep, for render-side interpolation.

public:
    /**
     * @brief Default constructor. Creates a Dynamic body with mass 1.
     */
    RigidBody();

    /**
     * @brief Constructor with body type and mass.
     * @param type Body type.
     * @param mass Mass in kg (ignored for Static/Kinematic).
     */
    RigidBody(BodyType type, float mass);

    // === Body Type ===

    BodyType GetBodyType() const { return bodyType_; }
    void SetBodyType(BodyType type);

    void MakeStatic()    { SetBodyType(BodyType::Static); }
    void MakeKinematic() { SetBodyType(BodyType::Kinematic); }
    void MakeDynamic()   { SetBodyType(BodyType::Dynamic); }
    bool IsStatic() const { return bodyType_ == BodyType::Static; }
    bool IsKinematic() const { return bodyType_ == BodyType::Kinematic; }
    bool IsDynamic() const { return bodyType_ == BodyType::Dynamic; }

    // === Mass ===

    float GetMass() const { return mass_; }
    float GetInverseMass() const { return inverseMass_; }
    void SetMass(float mass);

    // === Material Properties ===

    float GetRestitution() const { return restitution_; }
    void SetRestitution(float r) { restitution_ = r; }
    float GetFriction() const { return friction_; }
    void SetFriction(float f) { friction_ = f; }
    float GetLinearDamping() const { return linearDamping_; }
    void SetLinearDamping(float d) { linearDamping_ = d; }

    // === Velocity ===

    Vector3D GetVelocity() const { return velocity_; }
    void SetVelocity(const Vector3D& v) { velocity_ = v; Wake(); }
    /** Solver-internal: write velocity without waking the body (preserves sleep islands). */
    void SetVelocityRaw(const Vector3D& v) { velocity_ = v; }

    // === Angular state ===

    Vector3D GetAngularVelocity() const { return angularVelocity_; }
    void SetAngularVelocity(const Vector3D& w) { angularVelocity_ = w; Wake(); }
    /** Solver-internal: write angular velocity without waking the body. */
    void SetAngularVelocityRaw(const Vector3D& w) { angularVelocity_ = w; }

    float GetAngularDamping() const { return angularDamping_; }
    void SetAngularDamping(float d) { angularDamping_ = d; }

    // === Sleeping ===

    /** True while the body is asleep - solver treats it as static and integrator skips it. */
    bool IsSleeping() const { return sleeping_; }

    /**
     * @brief Wakes the body and resets the below-threshold timer. Call this on
     * any external impulse, force application, or when a sleeping body becomes
     * part of an island that contains an awake body.
     */
    void Wake() { sleeping_ = false; sleepTimer_ = 0.0f; }

    /** Forces the body asleep immediately (does not zero velocities). */
    void Sleep() { sleeping_ = true; }

    /** If false, this body is never put to sleep (e.g. player-controlled bodies). */
    bool GetAllowSleep() const { return allowSleep_; }
    void SetAllowSleep(bool allow) { allowSleep_ = allow; if (!allow) Wake(); }

    /** Internal: SleepManager uses these to track time-under-threshold. */
    float GetSleepTimer() const { return sleepTimer_; }
    void  SetSleepTimer(float t) { sleepTimer_ = t; }

    /** When true, broadphase inflates this body's AABB by an extra
     *  `(|v| + |ω|*r) * dt` so swept motion still discovers pairs the integrator
     *  would otherwise tunnel through. Speculative contact emission is also
     *  enabled for pairs involving this body. Default false (off);
     *  intended for fast-moving bodies (projectiles, debris) where tunneling
     *  is unacceptable. */
    bool IsBullet() const { return bullet_; }
    void SetBullet(bool b) { bullet_ = b; }

    /** Inertia tensors in the body's local frame. Zero -> angular response disabled. */
    const Matrix3x3& GetInertiaLocal() const { return inertiaLocal_; }
    const Matrix3x3& GetInverseInertiaLocal() const { return invInertiaLocal_; }
    /** World-space inverse inertia, recomputed every Integrate. */
    const Matrix3x3& GetInverseInertiaWorld() const { return invInertiaWorld_; }

    /**
     * @brief Set the local inertia tensor explicitly. Inverse and the world
     * tensor are recomputed.
     */
    void SetInertiaLocal(const Matrix3x3& I);

    /** Convenience setters using closed-form tensors (see InertiaTensor). */
    void SetInertiaSphere(float radius);
    void SetInertiaBox(const Vector3D& halfExtents);
    void SetInertiaCapsule(float radius, float halfHeight);
    void SetInertiaCylinder(float radius, float halfHeight);

    // === Forces ===

    /**
     * @brief Applies a force (accumulated until integration clears it).
     * @param force Force vector in Newtons.
     */
    void ApplyForce(const Vector3D& force);

    /**
     * @brief Applies an instantaneous velocity change.
     * @param impulse Impulse vector (kg·m/s).
     */
    void ApplyImpulse(const Vector3D& impulse);

    /**
     * @brief Accumulates a world-space torque (N·m) until the next Integrate.
     * No-op for non-Dynamic bodies.
     */
    void ApplyTorque(const Vector3D& torque);

    /**
     * @brief Applies a force at a world-space point. Generates both a linear
     * force and a torque about the body center of mass.
     */
    void ApplyForceAtPoint(const Vector3D& force, const Vector3D& worldPoint);

    /** Instantaneous angular velocity change. */
    void ApplyAngularImpulse(const Vector3D& angularImpulse);

    /** World-space velocity of a point rigidly attached to the body. */
    Vector3D GetPointVelocity(const Vector3D& worldPoint) const;

    /**
     * @brief Clears accumulated forces.
     */
    void ClearForces();

    // === Integration ===

    /**
     * @brief Integrates velocity and updates collider position.
     * Semi-implicit Euler: v += (F/m)*dt, then pos += v*dt.
     * @param dt Fixed timestep in seconds.
     * @param gravity World gravity vector.
     *
     * Equivalent to `IntegrateVelocity(dt, gravity); IntegratePosition(dt);`.
     * Prefer the split form in pipelines that need the solver to clamp
     * velocities before position advances (CCD, speculative contacts).
     */
    void Integrate(float dt, const Vector3D& gravity);

    /**
     * @brief Applies gravity + accumulated forces/torques to velocities only.
     * Pose is unchanged. Forces are NOT cleared (cleared by IntegratePosition).
     */
    void IntegrateVelocity(float dt, const Vector3D& gravity);

    /**
     * @brief Advances pose from current velocities and clears force accumulators.
     * Call after the solver has had a chance to modify velocities.
     */
    void IntegratePosition(float dt);

    // === Collider ===

    Collider* GetCollider() const { return collider_; }
    void SetCollider(Collider* c);

    // === Pose ===

    /**
     * @brief Returns the current world pose. Position is authoritative; orientation
     * is identity until angular dynamics is live.
     */
    const BodyPose& GetPose() const { return pose_; }

    /**
     * @brief Sets the world pose. Also updates collider position to match.
     */
    void SetPose(const BodyPose& pose);

    /**
     * @brief Pose recorded at the start of the most recent fixed substep.
     * Used by PhysicsWorld::GetInterpolatedPose for render-side smoothing.
     */
    const BodyPose& GetPreviousPose() const { return previousPose_; }

    /**
     * @brief Snapshot pose -> previousPose. Called by PhysicsWorld at the
     * top of each fixed substep.
     */
    void SnapshotPose() { previousPose_ = pose_; }

    KL_BEGIN_FIELDS(RigidBody)
        KL_FIELD(RigidBody, mass_, "Mass", 0, 0),
        KL_FIELD(RigidBody, restitution_, "Restitution", 0, 0),
        KL_FIELD(RigidBody, friction_, "Friction", 0, 0),
        KL_FIELD(RigidBody, linearDamping_, "Linear damping", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(RigidBody)
        KL_METHOD_AUTO(RigidBody, GetBodyType, "Get body type"),
        KL_METHOD_AUTO(RigidBody, MakeStatic, "Convert to static body"),
        KL_METHOD_AUTO(RigidBody, MakeKinematic, "Convert to kinematic body"),
        KL_METHOD_AUTO(RigidBody, MakeDynamic, "Convert to dynamic body"),
        KL_METHOD_AUTO(RigidBody, IsDynamic, "Is dynamic"),
        KL_METHOD_AUTO(RigidBody, IsStatic, "Is static"),
        KL_METHOD_AUTO(RigidBody, IsKinematic, "Is kinematic"),
        KL_METHOD_AUTO(RigidBody, GetMass, "Get mass"),
        KL_METHOD_AUTO(RigidBody, SetMass, "Set mass"),
        KL_METHOD_AUTO(RigidBody, GetVelocity, "Get velocity"),
        KL_METHOD_AUTO(RigidBody, SetVelocity, "Set velocity"),
        KL_METHOD_AUTO(RigidBody, GetAngularVelocity, "Get angular velocity"),
        KL_METHOD_AUTO(RigidBody, SetAngularVelocity, "Set angular velocity"),
        KL_METHOD_AUTO(RigidBody, ApplyForce, "Apply force"),
        KL_METHOD_AUTO(RigidBody, ApplyTorque, "Apply torque"),
        KL_METHOD_AUTO(RigidBody, ApplyForceAtPoint, "Apply force at point"),
        KL_METHOD_AUTO(RigidBody, ApplyImpulse, "Apply impulse"),
        KL_METHOD_AUTO(RigidBody, ApplyAngularImpulse, "Apply angular impulse"),
        KL_METHOD_AUTO(RigidBody, GetPointVelocity, "Get point velocity"),
        KL_METHOD_AUTO(RigidBody, ClearForces, "Clear forces"),
        KL_METHOD_AUTO(RigidBody, SetRestitution, "Set restitution"),
        KL_METHOD_AUTO(RigidBody, SetFriction, "Set friction"),
        KL_METHOD_AUTO(RigidBody, SetLinearDamping, "Set linear damping"),
        KL_METHOD_AUTO(RigidBody, SetAngularDamping, "Set angular damping"),
        KL_METHOD_AUTO(RigidBody, SetInertiaSphere, "Set inertia (sphere)"),
        KL_METHOD_AUTO(RigidBody, SetInertiaBox, "Set inertia (box)"),
        KL_METHOD_AUTO(RigidBody, SetInertiaCapsule, "Set inertia (capsule)"),
        KL_METHOD_AUTO(RigidBody, SetInertiaCylinder, "Set inertia (cylinder)"),
        KL_METHOD_AUTO(RigidBody, GetCollider, "Get collider"),
        KL_METHOD_AUTO(RigidBody, SetCollider, "Set collider"),
        KL_METHOD_AUTO(RigidBody, GetPose, "Get pose"),
        KL_METHOD_AUTO(RigidBody, SetPose, "Set pose"),
        KL_METHOD_AUTO(RigidBody, GetPreviousPose, "Get previous pose"),
        KL_METHOD_AUTO(RigidBody, IsSleeping, "Is sleeping"),
        KL_METHOD_AUTO(RigidBody, Wake, "Wake body"),
        KL_METHOD_AUTO(RigidBody, Sleep, "Put body to sleep"),
        KL_METHOD_AUTO(RigidBody, GetAllowSleep, "Get allow sleep"),
        KL_METHOD_AUTO(RigidBody, SetAllowSleep, "Set allow sleep")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(RigidBody)
        KL_CTOR0(RigidBody),
        KL_CTOR(RigidBody, BodyType, float)
    KL_END_DESCRIBE(RigidBody)
};

} // namespace koilo
