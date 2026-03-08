// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file physicsworld.hpp
 * @brief Physics world with fixed-timestep simulation, gravity, and impulse-based collision resolution.
 *
 * @date 21/02/2025
 * @author Coela Can't
 */

#pragma once

#include <vector>
#include <unordered_set>
#include <functional>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/collisionmanager.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct CollisionEvent
 * @brief Collision event data passed to script/C++ callbacks.
 */
struct CollisionEvent {
    Collider* colliderA;   ///< First collider
    Collider* colliderB;   ///< Second collider
    RigidBody* bodyA;      ///< First body (may be null)
    RigidBody* bodyB;      ///< Second body (may be null)
    Vector3D contactPoint; ///< Contact point
    Vector3D normal;       ///< Collision normal
    float penetration;     ///< Penetration depth

    CollisionEvent()
        : colliderA(nullptr), colliderB(nullptr), bodyA(nullptr), bodyB(nullptr),
          contactPoint(0,0,0), normal(0,1,0), penetration(0.0f) {}

    KL_BEGIN_FIELDS(CollisionEvent)
        KL_FIELD(CollisionEvent, colliderA, "Collider a", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(CollisionEvent)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CollisionEvent)
        /* No reflected ctors. */
    KL_END_DESCRIBE(CollisionEvent)

};

using PhysicsCollisionCallback = std::function<void(const CollisionEvent&)>;

/**
 * @class PhysicsWorld
 * @brief Manages a collection of rigid bodies with fixed-timestep integration and impulse collision resolution.
 *
 * Usage:
 *   PhysicsWorld world;
 *   world.AddBody(&body);
 *   // Each frame:
 *   world.Step(frameDeltaTime);
 */
class PhysicsWorld {
private:
    std::vector<RigidBody*> bodies_;     ///< Registered rigid bodies (non-owning)
    CollisionManager collisionManager_;   ///< Collision detection
    Vector3D gravity_;                    ///< World gravity
    float fixedDt_;                       ///< Fixed timestep (seconds)
    float accumulator_;                   ///< Time accumulator for fixed stepping
    int maxSubSteps_;                     ///< Max substeps per frame (prevents spiral of death)

    // Collision event tracking
    std::unordered_set<uint64_t> previousCollisions_;
    std::unordered_set<uint64_t> currentCollisions_;

    // Collision event callbacks
    std::vector<PhysicsCollisionCallback> onEnterCallbacks_;
    std::vector<PhysicsCollisionCallback> onStayCallbacks_;
    std::vector<PhysicsCollisionCallback> onExitCallbacks_;

    // Debug: last-frame contact info
    std::vector<CollisionEvent> debugContacts_;

    /**
     * @brief Performs one fixed-timestep physics step.
     */
    void FixedStep();

    /**
     * @brief Resolves collision between two bodies using impulse-based response.
     */
    void ResolveCollision(RigidBody* a, RigidBody* b, const CollisionInfo& info);

    /**
     * @brief Corrects penetration by pushing bodies apart (positional correction).
     */
    void CorrectPenetration(RigidBody* a, RigidBody* b, const CollisionInfo& info);

    /**
     * @brief Finds the RigidBody associated with a collider.
     */
    RigidBody* FindBodyForCollider(Collider* collider) const;

    /**
     * @brief Generates a unique pair ID for two colliders.
     */
    uint64_t GetPairID(Collider* a, Collider* b) const;

    /**
     * @brief Fires collision event callbacks.
     */
    void FireCollisionEvent(RigidBody* a, RigidBody* b, const CollisionInfo& info,
                           const std::vector<PhysicsCollisionCallback>& callbacks);

public:
    /**
     * @brief Default constructor. Gravity = (0, -9.81, 0), fixedDt = 1/60.
     */
    PhysicsWorld();

    /**
     * @brief Constructor with custom gravity.
     * @param gravity World gravity vector.
     */
    explicit PhysicsWorld(const Vector3D& gravity);

    // === Body Management ===

    void AddBody(RigidBody* body);
    void RemoveBody(RigidBody* body);
    void RemoveAllBodies();
    int GetBodyCount() const { return static_cast<int>(bodies_.size()); }
    RigidBody* GetBody(int index) const {
        if (index >= 0 && index < static_cast<int>(bodies_.size()))
            return bodies_[index];
        return nullptr;
    }

    // === Configuration ===

    Vector3D GetGravity() const { return gravity_; }
    void SetGravity(const Vector3D& g) { gravity_ = g; }
    float GetFixedTimestep() const { return fixedDt_; }
    void SetFixedTimestep(float dt) { fixedDt_ = dt; }
    int GetMaxSubSteps() const { return maxSubSteps_; }
    void SetMaxSubSteps(int steps) { maxSubSteps_ = steps; }

    // === Collision Manager Access ===

    CollisionManager& GetCollisionManager() { return collisionManager_; }

    // Debug contact access
    const std::vector<CollisionEvent>& GetDebugContacts() const { return debugContacts_; }
    int GetDebugContactCount() const { return static_cast<int>(debugContacts_.size()); }

    // === Collision Callbacks ===

    /**
     * @brief Registers a callback for collision enter events.
     */
    void OnCollisionEnter(PhysicsCollisionCallback callback);

    /**
     * @brief Registers a callback for collision stay events.
     */
    void OnCollisionStay(PhysicsCollisionCallback callback);

    /**
     * @brief Registers a callback for collision exit events.
     */
    void OnCollisionExit(PhysicsCollisionCallback callback);

    /**
     * @brief Clears all collision callbacks.
     */
    void ClearCollisionCallbacks();

    // === Simulation ===

    /**
     * @brief Advances the simulation by frameDt seconds.
     * Internally runs fixed-timestep substeps.
     * @param frameDt Frame delta time in seconds.
     */
    void Step();

    KL_BEGIN_FIELDS(PhysicsWorld)
        KL_FIELD(PhysicsWorld, gravity_, "Gravity", 0, 0),
        KL_FIELD(PhysicsWorld, fixedDt_, "Fixed timestep", 0, 0),
        KL_FIELD(PhysicsWorld, maxSubSteps_, "Max sub steps", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(PhysicsWorld)
        KL_METHOD_AUTO(PhysicsWorld, AddBody, "Add body"),
        KL_METHOD_AUTO(PhysicsWorld, RemoveBody, "Remove body"),
        KL_METHOD_AUTO(PhysicsWorld, RemoveAllBodies, "Remove all bodies"),
        KL_METHOD_AUTO(PhysicsWorld, GetBodyCount, "Get body count"),
        KL_METHOD_AUTO(PhysicsWorld, GetGravity, "Get gravity"),
        KL_METHOD_AUTO(PhysicsWorld, SetGravity, "Set gravity"),
        KL_METHOD_AUTO(PhysicsWorld, SetFixedTimestep, "Set fixed timestep"),
        KL_METHOD_AUTO(PhysicsWorld, SetMaxSubSteps, "Set max sub steps"),
        KL_METHOD_AUTO(PhysicsWorld, ClearCollisionCallbacks, "Clear collision callbacks"),
        KL_METHOD_AUTO(PhysicsWorld, Step, "Step")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(PhysicsWorld)
        KL_CTOR0(PhysicsWorld),
        KL_CTOR(PhysicsWorld, Vector3D)
    KL_END_DESCRIBE(PhysicsWorld)
};

} // namespace koilo
