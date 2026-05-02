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
#include <memory>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <koilo/systems/physics/collisionmanager.hpp>
#include <koilo/systems/physics/physics_budget.hpp>
#include <koilo/systems/physics/bodypose.hpp>
#include <koilo/systems/physics/contactcache.hpp>
#include <koilo/systems/physics/broadphase/broadphase.hpp>
#include <koilo/systems/physics/solver/sequentialimpulsesolver.hpp>
#include <koilo/systems/physics/solver/sleepmanager.hpp>
#include <koilo/systems/physics/physicsdiagnostics.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

class IShape;
class Joint;

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

/// Callback fired once per fixed substep, BEFORE integration. Receives the
/// substep's fixed dt. Used by external systems (aerodynamics, scripts) that
/// need to inject forces/impulses synchronously with the physics tick.
using PreFixedStepCallback = std::function<void(float)>;

/// Stable id returned by RegisterPreFixedStepCallback; pass back to
/// UnregisterPreFixedStepCallback. Zero is reserved for "none".
using PreFixedStepCallbackId = std::uint32_t;

inline constexpr PreFixedStepCallbackId kInvalidPreFixedStepCallbackId = 0;

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
    std::vector<Joint*>     joints_;     ///< Registered joints (non-owning)
    CollisionManager collisionManager_;   ///< Layer matrix + raycast query API
    Vector3D gravity_;                    ///< World gravity
    float fixedDt_;                       ///< Fixed timestep (seconds)
    float accumulator_;                   ///< Time accumulator for fixed stepping
    int maxSubSteps_;                     ///< Max substeps per Step() call (mirrors budget_.maxSubsteps)

    PhysicsBudget budget_;                ///< Real-time step budget
    PhysicsTransformPolicy transformPolicy_; ///< ECS<->physics sync direction
    DeterminismTier determinismTier_;     ///< Determinism guarantee level

    float interpolationAlpha_;            ///< Leftover time / fixedDt_ from last Step() for render-side smoothing
    int   lastStepSubsteps_;              ///< Substeps actually executed in the most recent Step()
    bool  lastStepOverran_;               ///< True if the most recent Step() hit a budget cap

    // Collision event tracking
    std::unordered_set<uint64_t> previousCollisions_;
    std::unordered_set<uint64_t> currentCollisions_;

    // Collision event callbacks
    std::vector<PhysicsCollisionCallback> onEnterCallbacks_;
    std::vector<PhysicsCollisionCallback> onStayCallbacks_;
    std::vector<PhysicsCollisionCallback> onExitCallbacks_;

    struct PreFixedStepEntry {
        PreFixedStepCallbackId id;
        PreFixedStepCallback   fn;
    };
    std::vector<PreFixedStepEntry> preFixedStepCallbacks_;
    PreFixedStepCallbackId         nextPreFixedStepId_ = 1;

    // Debug: last-frame contact info
    std::vector<CollisionEvent> debugContacts_;

    // === manifold pipeline ===
    std::vector<std::unique_ptr<IShape>>        shapes_;     ///< Parallel to bodies_; null if collider has no proxy
    std::vector<std::unique_ptr<ColliderProxy>> proxies_;    ///< Parallel to bodies_; null if no shape
    Broadphase                broadphase_;
    ContactCache              contactCache_;
    SequentialImpulseSolver   solver_;
    SolverConfig              solverConfig_;
    SleepConfig               sleepConfig_;

    // === CCD ===
    /** Base broadphase margin used for non-bullet bodies. Mirrors the DBVT default. */
    float baseBroadphaseMargin_ = 0.05f;
    /** Maximum total swept margin (m). Bullet swept margin is clamped to this so
     *  pathological all-bullet scenes don't blow up the broadphase. */
    float maxSweptMargin_       = 4.0f;
    /** Telemetry: number of bullet-flagged proxies in the most recent FixedStep. */
    int   lastStepBulletProxies_ = 0;
    /** Telemetry: number of speculative manifolds emitted this FixedStep. */
    int   lastStepSpeculativeManifolds_ = 0;

    /**
     * @brief Performs one fixed-timestep physics step using the manifold pipeline.
     */
    void FixedStep();

    /**
     * @brief Builds an IShape from a Collider's type + dimensions. Returns null
     * if the collider type is Custom or unsupported. Owns the returned shape.
     */
    std::unique_ptr<IShape> BuildShapeFromCollider(Collider* collider) const;

    /**
     * @brief Creates a proxy + shape for body[index] from its collider. No-op
     * if the body has no collider or the collider type is unsupported.
     */
    void RegisterProxyForBody(std::size_t index);

    /**
     * @brief Refreshes bodyId on every proxy after a body is removed (since
     * proxies index into bodies_ which has shifted).
     */
    void RebuildProxyBodyIds();

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

    // === Joint Management ===

    /**
     * @brief Register a joint (caller-owned). Joints are solved alongside
     * contacts each step. Re-adding the same joint pointer is a no-op.
     */
    void AddJoint(Joint* joint);

    /**
     * @brief Unregister a joint. Call before destroying the Joint object.
     * Removing a joint not currently registered is a no-op.
     */
    void RemoveJoint(Joint* joint);

    int GetJointCount() const { return static_cast<int>(joints_.size()); }
    Joint* GetJoint(int index) const {
        if (index >= 0 && index < static_cast<int>(joints_.size())) return joints_[index];
        return nullptr;
    }

    // === Configuration ===

    Vector3D GetGravity() const { return gravity_; }
    void SetGravity(const Vector3D& g) { gravity_ = g; }
    float GetFixedTimestep() const { return fixedDt_; }
    void SetFixedTimestep(float dt) { fixedDt_ = dt; }
    int GetMaxSubSteps() const { return maxSubSteps_; }
    void SetMaxSubSteps(int steps) { maxSubSteps_ = steps; budget_.maxSubsteps = steps; }

    // === Real-time budget / policy / determinism ===

    const PhysicsBudget& GetBudget() const { return budget_; }
    void SetBudget(const PhysicsBudget& b) { budget_ = b; maxSubSteps_ = b.maxSubsteps; }

    PhysicsTransformPolicy GetTransformPolicy() const { return transformPolicy_; }
    void SetTransformPolicy(PhysicsTransformPolicy p) { transformPolicy_ = p; }

    DeterminismTier GetDeterminismTier() const { return determinismTier_; }
    void SetDeterminismTier(DeterminismTier t) { determinismTier_ = t; }

    /**
     * @brief Fractional residue of accumulator after the most recent Step(),
     * normalized to [0,1) by fixedDt_. Use for render-side pose interpolation.
     */
    float GetInterpolationAlpha() const { return interpolationAlpha_; }

    int  GetLastStepSubsteps() const { return lastStepSubsteps_; }
    bool DidLastStepOverrun() const { return lastStepOverran_; }

    /**
     * @brief Pose of a body interpolated between its previous-substep and
     * current-substep state. Pass GetInterpolationAlpha() for smooth render.
     */
    BodyPose GetInterpolatedPose(const RigidBody* body, float alpha) const;

    // === Collision Manager Access ===

    CollisionManager& GetCollisionManager() { return collisionManager_; }

    // === Solver / sleep tuning ===

    SolverConfig& GetSolverConfig() { return solverConfig_; }
    const SolverConfig& GetSolverConfig() const { return solverConfig_; }
    SleepConfig&  GetSleepConfig()  { return sleepConfig_; }
    const SleepConfig&  GetSleepConfig()  const { return sleepConfig_; }

    // === CCD ===
    float GetBaseBroadphaseMargin() const { return baseBroadphaseMargin_; }
    void  SetBaseBroadphaseMargin(float m) { baseBroadphaseMargin_ = m; }
    float GetMaxSweptMargin() const { return maxSweptMargin_; }
    void  SetMaxSweptMargin(float m) { maxSweptMargin_ = m; }
    int   GetLastStepBulletProxies() const { return lastStepBulletProxies_; }
    int   GetLastStepSpeculativeManifolds() const { return lastStepSpeculativeManifolds_; }

    // === Conservation diagnostics ===
    /**
     * @brief Snapshot of total linear momentum, angular momentum (about world
     * origin), and kinetic energy of all dynamic bodies. O(N), called on
     * demand. Static/kinematic bodies are excluded.
     */
    PhysicsDiagnostics ComputeDiagnostics() const;
    const ContactCache& GetContactCache() const { return contactCache_; }
    const Broadphase&   GetBroadphase()   const { return broadphase_; }

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

    // === Pre-fixed-step callbacks ===

    /**
     * @brief Registers a callback fired once per fixed substep, BEFORE
     * integration. The callback receives the substep's fixed dt. Multiple
     * callbacks may be registered; they fire in registration order.
     * @return Stable id usable with UnregisterPreFixedStepCallback.
     */
    PreFixedStepCallbackId RegisterPreFixedStepCallback(PreFixedStepCallback cb);

    /**
     * @brief Unregisters a previously registered pre-fixed-step callback.
     * Unknown or `kInvalidPreFixedStepCallbackId` is a safe no-op.
     */
    void UnregisterPreFixedStepCallback(PreFixedStepCallbackId id);

    /**
     * @brief Removes ALL pre-fixed-step callbacks. Convenience for tests.
     */
    void ClearPreFixedStepCallbacks();

    // === Simulation ===

    /**
     * @brief Advances the simulation by frameDt seconds.
     *
     * Runs fixed-timestep substeps (fixedDt_ each) until the accumulated
     * time is below fixedDt_ or the budget (PhysicsBudget) is exhausted.
     * On budget overrun any remaining substep time is dropped (latency over
     * catch-up - kills the spiral of death). Determinism Tier 1 requires
     * that you pass the same dt sequence for replay.
     *
     * @param frameDt Frame delta time in seconds.
     */
    void Step(float frameDt);

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
        KL_METHOD_AUTO(PhysicsWorld, Step, "Step"),
        KL_METHOD_AUTO(PhysicsWorld, ComputeDiagnostics, "Compute conservation diagnostics"),
        KL_METHOD_AUTO(PhysicsWorld, GetDebugContactCount, "Get debug contact count")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(PhysicsWorld)
        KL_CTOR0(PhysicsWorld),
        KL_CTOR(PhysicsWorld, Vector3D)
    KL_END_DESCRIBE(PhysicsWorld)
};

} // namespace koilo
