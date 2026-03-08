// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file collisionmanager.hpp
 * @brief Collision detection and management system.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <vector>
#include <unordered_set>
#include <functional>
#include "collider.hpp"
#include "spherecollider.hpp"
#include "boxcollider.hpp"
#include "capsulecollider.hpp"
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct CollisionInfo
 * @brief Information about a collision between two colliders.
 */
struct CollisionInfo {
    Collider* colliderA;          ///< First collider
    Collider* colliderB;          ///< Second collider
    Vector3D contactPoint;        ///< Contact point in world space
    Vector3D normal;              ///< Collision normal (from A to B)
    float penetrationDepth;       ///< How deep the collision is

    /**
     * @brief Default constructor.
     */
    CollisionInfo()
        : colliderA(nullptr), colliderB(nullptr),
          contactPoint(0, 0, 0), normal(0, 1, 0), penetrationDepth(0.0f) {}

    KL_BEGIN_FIELDS(CollisionInfo)
        KL_FIELD(CollisionInfo, contactPoint, "Contact point", 0, 0),
        KL_FIELD(CollisionInfo, normal, "Normal", 0, 0),
        KL_FIELD(CollisionInfo, penetrationDepth, "Penetration depth", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(CollisionInfo)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CollisionInfo)
        KL_CTOR0(CollisionInfo)
    KL_END_DESCRIBE(CollisionInfo)
};

/**
 * @typedef CollisionCallback
 * @brief Callback function type for collision events.
 */
using CollisionCallback = std::function<void(const CollisionInfo&)>;

/**
 * @class CollisionManager
 * @brief Manages collision detection between registered colliders.
 */
class CollisionManager {
private:
    std::vector<Collider*> colliders;                    ///< Registered colliders
    bool collisionMatrix[32][32];                        ///< Layer collision matrix
    std::unordered_set<uint64_t> previousCollisions;    ///< Collisions from last frame
    std::unordered_set<uint64_t> currentCollisions;     ///< Collisions this frame

    std::vector<CollisionCallback> onCollisionEnterCallbacks;  ///< Enter callbacks
    std::vector<CollisionCallback> onCollisionStayCallbacks;   ///< Stay callbacks
    std::vector<CollisionCallback> onCollisionExitCallbacks;   ///< Exit callbacks

public:
    /**
     * @brief Constructor.
     */
    CollisionManager();

    /**
     * @brief Destructor.
     */
    ~CollisionManager();

    // === Collider Management ===

    /**
     * @brief Registers a collider for collision detection.
     * @param collider Pointer to collider.
     */
    void RegisterCollider(Collider* collider);

    /**
     * @brief Unregisters a collider.
     * @param collider Pointer to collider.
     */
    void UnregisterCollider(Collider* collider);

    /**
     * @brief Unregisters all colliders.
     */
    void UnregisterAllColliders();

    /**
     * @brief Gets the number of registered colliders.
     */
    size_t GetColliderCount() const { return colliders.size(); }

    // === Collision Matrix ===

    /**
     * @brief Sets whether two layers can collide.
     * @param layerA First layer (0-31).
     * @param layerB Second layer (0-31).
     * @param canCollide True if layers can collide.
     */
    void SetLayerCollision(int layerA, int layerB, bool canCollide);

    /**
     * @brief Checks if two layers can collide.
     */
    bool CanLayersCollide(int layerA, int layerB) const;

    /**
     * @brief Sets default collision matrix (all layers collide with all layers).
     */
    void SetDefaultCollisionMatrix();

    // === Collision Detection ===

    /**
     * @brief Updates collision detection (call once per frame).
     */
    void Update();

    /**
     * @brief Tests collision between two specific colliders.
     * @param a First collider.
     * @param b Second collider.
     * @param info Output collision information.
     * @return True if colliders are intersecting.
     */
    bool TestCollision(Collider* a, Collider* b, CollisionInfo& info);

    // === Raycasting ===

    /**
     * @brief Casts a ray and finds the closest hit.
     * @param origin Ray origin.
     * @param direction Ray direction (must be normalized).
     * @param hit Output hit information.
     * @param maxDistance Maximum ray distance.
     * @param layerMask Layer mask for filtering (default: all layers).
     * @return True if ray hit something.
     */
    bool Raycast(const Vector3D& origin, const Vector3D& direction,
                 RaycastHit& hit, float maxDistance,
                 uint32_t layerMask = 0xFFFFFFFF);

    /**
     * @brief Casts a ray and finds all hits.
     * @param origin Ray origin.
     * @param direction Ray direction (must be normalized).
     * @param hits Output vector of hit information.
     * @param maxDistance Maximum ray distance.
     * @param layerMask Layer mask for filtering (default: all layers).
     * @return Number of hits.
     */
    int RaycastAll(const Vector3D& origin, const Vector3D& direction,
                   std::vector<RaycastHit>& hits, float maxDistance,
                   uint32_t layerMask = 0xFFFFFFFF);

    // === Overlap Queries ===

    /**
     * @brief Checks if a sphere overlaps any collider.
     * @param center Sphere center.
     * @param radius Sphere radius.
     * @param layerMask Layer mask for filtering (default: all layers).
     * @return True if overlap detected.
     */
    bool OverlapSphere(const Vector3D& center, float radius,
                       uint32_t layerMask = 0xFFFFFFFF);

    /**
     * @brief Gets all colliders overlapping a sphere.
     * @param center Sphere center.
     * @param radius Sphere radius.
     * @param results Output vector of overlapping colliders.
     * @param layerMask Layer mask for filtering (default: all layers).
     * @return Number of overlapping colliders.
     */
    int OverlapSphereAll(const Vector3D& center, float radius,
                         std::vector<Collider*>& results,
                         uint32_t layerMask = 0xFFFFFFFF);

    /**
     * @brief Checks if a box overlaps any collider.
     * @param center Box center.
     * @param extents Box half-size.
     * @param layerMask Layer mask for filtering (default: all layers).
     * @return True if overlap detected.
     */
    bool OverlapBox(const Vector3D& center, const Vector3D& extents,
                    uint32_t layerMask = 0xFFFFFFFF);

    // === Callbacks ===

    /**
     * @brief Adds a collision enter callback.
     */
    void AddCollisionEnterCallback(CollisionCallback callback);

    /**
     * @brief Adds a collision stay callback.
     */
    void AddCollisionStayCallback(CollisionCallback callback);

    /**
     * @brief Adds a collision exit callback.
     */
    void AddCollisionExitCallback(CollisionCallback callback);

    /**
     * @brief Clears all callbacks.
     */
    void ClearCallbacks();

private:
    /**
     * @brief Broadphase collision detection (generates potential pairs).
     */
    void BroadPhase(std::vector<std::pair<Collider*, Collider*>>& pairs);

    /**
     * @brief Narrowphase collision detection (tests pairs).
     */
    void NarrowPhase(const std::vector<std::pair<Collider*, Collider*>>& pairs);

    /**
     * @brief Gets a unique ID for a collider pair.
     */
    uint64_t GetPairID(Collider* a, Collider* b) const;

    /**
     * @brief Checks if a layer is in a layer mask.
     */
    bool IsLayerInMask(int layer, uint32_t mask) const;

    /**
     * @brief Tests sphere-sphere collision.
     */
    bool TestSphereSphere(SphereCollider* a, SphereCollider* b, CollisionInfo& info);

    /**
     * @brief Tests sphere-box collision.
     */
    bool TestSphereBox(SphereCollider* sphere, BoxCollider* box, CollisionInfo& info);

    /**
     * @brief Tests box-box collision.
     */
    bool TestBoxBox(BoxCollider* a, BoxCollider* b, CollisionInfo& info);

    /**
     * @brief Tests capsule-sphere collision.
     */
    bool TestCapsuleSphere(CapsuleCollider* capsule, SphereCollider* sphere, CollisionInfo& info);

    /**
     * @brief Tests capsule-capsule collision.
     */
    bool TestCapsuleCapsule(CapsuleCollider* a, CapsuleCollider* b, CollisionInfo& info);

    /**
     * @brief Tests capsule-box collision.
     */
    bool TestCapsuleBox(CapsuleCollider* capsule, BoxCollider* box, CollisionInfo& info);

    /**
     * @brief Returns closest point on line segment ab to point p.
     */
    static Vector3D ClosestPointOnSegment(const Vector3D& p, const Vector3D& a, const Vector3D& b);

    KL_BEGIN_FIELDS(CollisionManager)
    KL_END_FIELDS

    KL_BEGIN_METHODS(CollisionManager)
        KL_METHOD_AUTO(CollisionManager, RegisterCollider, "Register collider"),
        KL_METHOD_AUTO(CollisionManager, UnregisterCollider, "Unregister collider"),
        KL_METHOD_AUTO(CollisionManager, UnregisterAllColliders, "Unregister all colliders"),
        KL_METHOD_AUTO(CollisionManager, SetLayerCollision, "Set layer collision"),
        KL_METHOD_AUTO(CollisionManager, CanLayersCollide, "Can layers collide"),
        KL_METHOD_AUTO(CollisionManager, SetDefaultCollisionMatrix, "Set default collision matrix"),
        KL_METHOD_AUTO(CollisionManager, Update, "Update"),
        KL_METHOD_AUTO(CollisionManager, ClearCallbacks, "Clear callbacks")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CollisionManager)
        KL_CTOR0(CollisionManager)
    KL_END_DESCRIBE(CollisionManager)
};

} // namespace koilo
