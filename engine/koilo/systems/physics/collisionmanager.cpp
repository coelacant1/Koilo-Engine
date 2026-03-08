#include <ptx/systems/physics/collisionmanager.hpp>
#include <algorithm>
#include <cmath>

namespace ptx {

CollisionManager::CollisionManager() {
    SetDefaultCollisionMatrix();
}

CollisionManager::~CollisionManager() {
    UnregisterAllColliders();
}

// === Collider Management ===

void CollisionManager::RegisterCollider(Collider* collider) {
    if (collider == nullptr) return;

    // Check if already registered
    auto it = std::find(colliders.begin(), colliders.end(), collider);
    if (it == colliders.end()) {
        colliders.push_back(collider);
    }
}

void CollisionManager::UnregisterCollider(Collider* collider) {
    if (collider == nullptr) return;

    colliders.erase(
        std::remove(colliders.begin(), colliders.end(), collider),
        colliders.end()
    );
}

void CollisionManager::UnregisterAllColliders() {
    colliders.clear();
}

// === Collision Matrix ===

void CollisionManager::SetLayerCollision(int layerA, int layerB, bool canCollide) {
    if (layerA < 0 || layerA >= 32 || layerB < 0 || layerB >= 32) return;

    collisionMatrix[layerA][layerB] = canCollide;
    collisionMatrix[layerB][layerA] = canCollide;
}

bool CollisionManager::CanLayersCollide(int layerA, int layerB) const {
    if (layerA < 0 || layerA >= 32 || layerB < 0 || layerB >= 32) return false;

    return collisionMatrix[layerA][layerB];
}

void CollisionManager::SetDefaultCollisionMatrix() {
    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 32; ++j) {
            collisionMatrix[i][j] = true;
        }
    }
}

// === Collision Detection ===

void CollisionManager::Update() {
    currentCollisions.clear();

    // Broadphase: generate potential collision pairs
    std::vector<std::pair<Collider*, Collider*>> pairs;
    BroadPhase(pairs);

    // Narrowphase: test collision pairs
    NarrowPhase(pairs);

    // Update collision states for next frame
    previousCollisions = currentCollisions;
}

void CollisionManager::BroadPhase(std::vector<std::pair<Collider*, Collider*>>& pairs) {
    // Simple O(n^2) broadphase
    // TODO: Implement spatial partitioning (octree, BVH) for better performance

    for (size_t i = 0; i < colliders.size(); ++i) {
        if (!colliders[i]->IsEnabled()) continue;

        for (size_t j = i + 1; j < colliders.size(); ++j) {
            if (!colliders[j]->IsEnabled()) continue;

            // Check layer collision
            if (!CanLayersCollide(colliders[i]->GetLayer(), colliders[j]->GetLayer())) {
                continue;
            }

            // Simple AABB bounds check would go here
            // For now, add all pairs (narrowphase will filter)
            pairs.emplace_back(colliders[i], colliders[j]);
        }
    }
}

void CollisionManager::NarrowPhase(const std::vector<std::pair<Collider*, Collider*>>& pairs) {
    for (const auto& pair : pairs) {
        CollisionInfo info;
        if (TestCollision(pair.first, pair.second, info)) {
            uint64_t pairID = GetPairID(pair.first, pair.second);
            currentCollisions.insert(pairID);

            // Check if this is a new collision (enter) or ongoing (stay)
            if (previousCollisions.find(pairID) == previousCollisions.end()) {
                // Collision enter
                for (auto& callback : onCollisionEnterCallbacks) {
                    callback(info);
                }
            } else {
                // Collision stay
                for (auto& callback : onCollisionStayCallbacks) {
                    callback(info);
                }
            }
        }
    }

    // Check for collision exits
    for (uint64_t pairID : previousCollisions) {
        if (currentCollisions.find(pairID) == currentCollisions.end()) {
            // Collision exit (info not available, create empty)
            CollisionInfo info;
            for (auto& callback : onCollisionExitCallbacks) {
                callback(info);
            }
        }
    }
}

bool CollisionManager::TestCollision(Collider* a, Collider* b, CollisionInfo& info) {
    if (a == nullptr || b == nullptr) return false;

    info.colliderA = a;
    info.colliderB = b;

    // Dispatch based on collider types
    if (a->GetType() == ColliderType::Sphere && b->GetType() == ColliderType::Sphere) {
        return TestSphereSphere(static_cast<SphereCollider*>(a),
                               static_cast<SphereCollider*>(b), info);
    }
    else if (a->GetType() == ColliderType::Sphere && b->GetType() == ColliderType::Box) {
        return TestSphereBox(static_cast<SphereCollider*>(a),
                            static_cast<BoxCollider*>(b), info);
    }
    else if (a->GetType() == ColliderType::Box && b->GetType() == ColliderType::Sphere) {
        bool result = TestSphereBox(static_cast<SphereCollider*>(b),
                                    static_cast<BoxCollider*>(a), info);
        if (result) {
            std::swap(info.colliderA, info.colliderB);
            info.normal = info.normal * -1.0f;
        }
        return result;
    }
    else if (a->GetType() == ColliderType::Box && b->GetType() == ColliderType::Box) {
        return TestBoxBox(static_cast<BoxCollider*>(a),
                         static_cast<BoxCollider*>(b), info);
    }

    // Unsupported collision type combination
    return false;
}

// === Raycasting ===

bool CollisionManager::Raycast(const Vector3D& origin, const Vector3D& direction,
                               RaycastHit& hit, float maxDistance, uint32_t layerMask) {
    bool foundHit = false;
    float closestDistance = maxDistance;

    for (auto* collider : colliders) {
        if (!collider->IsEnabled()) continue;
        if (!IsLayerInMask(collider->GetLayer(), layerMask)) continue;

        RaycastHit tempHit;
        if (collider->Raycast(origin, direction, tempHit, maxDistance)) {
            if (tempHit.distance < closestDistance) {
                closestDistance = tempHit.distance;
                hit = tempHit;
                foundHit = true;
            }
        }
    }

    return foundHit;
}

int CollisionManager::RaycastAll(const Vector3D& origin, const Vector3D& direction,
                                 std::vector<RaycastHit>& hits, float maxDistance,
                                 uint32_t layerMask) {
    hits.clear();

    for (auto* collider : colliders) {
        if (!collider->IsEnabled()) continue;
        if (!IsLayerInMask(collider->GetLayer(), layerMask)) continue;

        RaycastHit hit;
        if (collider->Raycast(origin, direction, hit, maxDistance)) {
            hits.push_back(hit);
        }
    }

    // Sort by distance
    std::sort(hits.begin(), hits.end(),
        [](const RaycastHit& a, const RaycastHit& b) {
            return a.distance < b.distance;
        });

    return static_cast<int>(hits.size());
}

// === Overlap Queries ===

bool CollisionManager::OverlapSphere(const Vector3D& center, float radius,
                                     uint32_t layerMask) {
    for (auto* collider : colliders) {
        if (!collider->IsEnabled()) continue;
        if (!IsLayerInMask(collider->GetLayer(), layerMask)) continue;

        Vector3D closest = collider->ClosestPoint(center);
        float dist = (closest - center).length();
        if (dist <= radius) {
            return true;
        }
    }

    return false;
}

int CollisionManager::OverlapSphereAll(const Vector3D& center, float radius,
                                       std::vector<Collider*>& results,
                                       uint32_t layerMask) {
    results.clear();

    for (auto* collider : colliders) {
        if (!collider->IsEnabled()) continue;
        if (!IsLayerInMask(collider->GetLayer(), layerMask)) continue;

        Vector3D closest = collider->ClosestPoint(center);
        float dist = (closest - center).length();
        if (dist <= radius) {
            results.push_back(collider);
        }
    }

    return static_cast<int>(results.size());
}

bool CollisionManager::OverlapBox(const Vector3D& center, const Vector3D& extents,
                                  uint32_t layerMask) {
    // Create a temporary box collider for testing
    BoxCollider queryBox(center, extents);

    for (auto* collider : colliders) {
        if (!collider->IsEnabled()) continue;
        if (!IsLayerInMask(collider->GetLayer(), layerMask)) continue;

        CollisionInfo info;
        if (TestCollision(&queryBox, collider, info)) {
            return true;
        }
    }

    return false;
}

// === Callbacks ===

void CollisionManager::AddCollisionEnterCallback(CollisionCallback callback) {
    onCollisionEnterCallbacks.push_back(callback);
}

void CollisionManager::AddCollisionStayCallback(CollisionCallback callback) {
    onCollisionStayCallbacks.push_back(callback);
}

void CollisionManager::AddCollisionExitCallback(CollisionCallback callback) {
    onCollisionExitCallbacks.push_back(callback);
}

void CollisionManager::ClearCallbacks() {
    onCollisionEnterCallbacks.clear();
    onCollisionStayCallbacks.clear();
    onCollisionExitCallbacks.clear();
}

// === Private Helper Methods ===

uint64_t CollisionManager::GetPairID(Collider* a, Collider* b) const {
    uintptr_t ptrA = reinterpret_cast<uintptr_t>(a);
    uintptr_t ptrB = reinterpret_cast<uintptr_t>(b);

    // Ensure consistent ordering
    if (ptrA > ptrB) std::swap(ptrA, ptrB);

    return (static_cast<uint64_t>(ptrA) << 32) | static_cast<uint64_t>(ptrB);
}

bool CollisionManager::IsLayerInMask(int layer, uint32_t mask) const {
    return (mask & (1 << layer)) != 0;
}

bool CollisionManager::TestSphereSphere(SphereCollider* a, SphereCollider* b,
                                       CollisionInfo& info) {
    Vector3D delta = b->GetPosition() - a->GetPosition();
    float dist = delta.length();
    float radiusSum = a->GetRadius() + b->GetRadius();

    if (dist < radiusSum) {
        info.penetrationDepth = radiusSum - dist;
        info.normal = delta;
        info.normal.normalize();
        info.contactPoint = a->GetPosition() + info.normal * a->GetRadius();
        return true;
    }

    return false;
}

bool CollisionManager::TestSphereBox(SphereCollider* sphere, BoxCollider* box,
                                     CollisionInfo& info) {
    Vector3D closest = box->ClosestPoint(sphere->GetPosition());
    Vector3D delta = sphere->GetPosition() - closest;
    float dist = delta.length();

    if (dist < sphere->GetRadius()) {
        info.penetrationDepth = sphere->GetRadius() - dist;
        info.normal = delta;
        info.normal.normalize();
        info.contactPoint = closest;
        return true;
    }

    return false;
}

bool CollisionManager::TestBoxBox(BoxCollider* a, BoxCollider* b, CollisionInfo& info) {
    // Simplified AABB-AABB test
    Vector3D minA = a->GetMinimum();
    Vector3D maxA = a->GetMaximum();
    Vector3D minB = b->GetMinimum();
    Vector3D maxB = b->GetMaximum();

    // Check for overlap on all axes
    bool overlapX = (minA.x <= maxB.x && maxA.x >= minB.x);
    bool overlapY = (minA.y <= maxB.y && maxA.y >= minB.y);
    bool overlapZ = (minA.z <= maxB.z && maxA.z >= minB.z);

    if (overlapX && overlapY && overlapZ) {
        // Calculate penetration and contact info (simplified)
        Vector3D centerA = a->GetPosition();
        Vector3D centerB = b->GetPosition();

        info.normal = centerB - centerA;
        info.normal.normalize();
        info.contactPoint = (centerA + centerB) * 0.5f;
        info.penetrationDepth = 0.1f;  // Simplified

        return true;
    }

    return false;
}

} // namespace ptx
