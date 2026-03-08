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
    Collider* collider_;    ///< Associated collider (non-owning)

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
    void SetVelocity(const Vector3D& v) { velocity_ = v; }

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
     * @brief Clears accumulated forces.
     */
    void ClearForces();

    // === Integration ===

    /**
     * @brief Integrates velocity and updates collider position.
     * Semi-implicit Euler: v += (F/m)*dt, then pos += v*dt.
     * @param dt Fixed timestep in seconds.
     * @param gravity World gravity vector.
     */
    void Integrate(float dt, const Vector3D& gravity);

    // === Collider ===

    Collider* GetCollider() const { return collider_; }
    void SetCollider(Collider* c) { collider_ = c; }

    KL_BEGIN_FIELDS(RigidBody)
        KL_FIELD(RigidBody, mass_, "Mass", 0, 0),
        KL_FIELD(RigidBody, restitution_, "Restitution", 0, 0),
        KL_FIELD(RigidBody, friction_, "Friction", 0, 0),
        KL_FIELD(RigidBody, linearDamping_, "Linear damping", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(RigidBody)
        KL_METHOD_AUTO(RigidBody, GetBodyType, "Get body type"),
        KL_METHOD_AUTO(RigidBody, IsDynamic, "Is dynamic"),
        KL_METHOD_AUTO(RigidBody, IsStatic, "Is static"),
        KL_METHOD_AUTO(RigidBody, IsKinematic, "Is kinematic"),
        KL_METHOD_AUTO(RigidBody, GetMass, "Get mass"),
        KL_METHOD_AUTO(RigidBody, SetMass, "Set mass"),
        KL_METHOD_AUTO(RigidBody, GetVelocity, "Get velocity"),
        KL_METHOD_AUTO(RigidBody, SetVelocity, "Set velocity"),
        KL_METHOD_AUTO(RigidBody, ApplyForce, "Apply force"),
        KL_METHOD_AUTO(RigidBody, ApplyImpulse, "Apply impulse"),
        KL_METHOD_AUTO(RigidBody, ClearForces, "Clear forces"),
        KL_METHOD_AUTO(RigidBody, SetRestitution, "Set restitution"),
        KL_METHOD_AUTO(RigidBody, SetFriction, "Set friction"),
        KL_METHOD_AUTO(RigidBody, SetLinearDamping, "Set linear damping")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(RigidBody)
        KL_CTOR0(RigidBody),
        KL_CTOR(RigidBody, BodyType, float)
    KL_END_DESCRIBE(RigidBody)
};

} // namespace koilo
