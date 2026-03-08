// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/physics/collisionmanager.hpp>
#include <algorithm>

namespace koilo {

koilo::CollisionManager::CollisionManager() {
    SetDefaultCollisionMatrix();
}

koilo::CollisionManager::~CollisionManager() {
    UnregisterAllColliders();
}

// === Collider Management ===

void koilo::CollisionManager::RegisterCollider(Collider* collider) {
    if (collider == nullptr) return;

    // Check if already registered
    auto it = std::find(colliders.begin(), colliders.end(), collider);
    if (it == colliders.end()) {
        colliders.push_back(collider);
    }
}

void koilo::CollisionManager::UnregisterCollider(Collider* collider) {
    if (collider == nullptr) return;

    colliders.erase(
        std::remove(colliders.begin(), colliders.end(), collider),
        colliders.end()
    );
}

void koilo::CollisionManager::UnregisterAllColliders() {
    colliders.clear();
}

// === Collision Matrix ===

void koilo::CollisionManager::SetLayerCollision(int layerA, int layerB, bool canCollide) {
    if (layerA < 0 || layerA >= 32 || layerB < 0 || layerB >= 32) return;

    collisionMatrix[layerA][layerB] = canCollide;
    collisionMatrix[layerB][layerA] = canCollide;
}

bool koilo::CollisionManager::CanLayersCollide(int layerA, int layerB) const {
    if (layerA < 0 || layerA >= 32 || layerB < 0 || layerB >= 32) return false;

    return collisionMatrix[layerA][layerB];
}

void koilo::CollisionManager::SetDefaultCollisionMatrix() {
    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 32; ++j) {
            collisionMatrix[i][j] = true;
        }
    }
}

// === Collision Detection ===

void koilo::CollisionManager::Update() {
    currentCollisions.clear();

    // Broadphase: generate potential collision pairs
    std::vector<std::pair<Collider*, Collider*>> pairs;
    BroadPhase(pairs);

    // Narrowphase: test collision pairs
    NarrowPhase(pairs);

    // Update collision states for next frame
    previousCollisions = currentCollisions;
}

void koilo::CollisionManager::BroadPhase(std::vector<std::pair<Collider*, Collider*>>& pairs) {
    // Simple O(n^2) broadphase
    // TODO: Spatial partitioning (octree/BVH) for broadphase

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

void koilo::CollisionManager::NarrowPhase(const std::vector<std::pair<Collider*, Collider*>>& pairs) {
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

bool koilo::CollisionManager::TestCollision(Collider* a, Collider* b, CollisionInfo& info) {
    if (a == nullptr || b == nullptr) return false;

    info.colliderA = a;
    info.colliderB = b;

    ColliderType ta = a->GetType();
    ColliderType tb = b->GetType();

    // Sphere-Sphere
    if (ta == ColliderType::Sphere && tb == ColliderType::Sphere) {
        return TestSphereSphere(static_cast<SphereCollider*>(a),
                               static_cast<SphereCollider*>(b), info);
    }
    // Sphere-Box (ordered)
    if (ta == ColliderType::Sphere && tb == ColliderType::Box) {
        return TestSphereBox(static_cast<SphereCollider*>(a),
                            static_cast<BoxCollider*>(b), info);
    }
    if (ta == ColliderType::Box && tb == ColliderType::Sphere) {
        bool result = TestSphereBox(static_cast<SphereCollider*>(b),
                                    static_cast<BoxCollider*>(a), info);
        if (result) {
            std::swap(info.colliderA, info.colliderB);
            info.normal = info.normal * -1.0f;
        }
        return result;
    }
    // Box-Box
    if (ta == ColliderType::Box && tb == ColliderType::Box) {
        return TestBoxBox(static_cast<BoxCollider*>(a),
                         static_cast<BoxCollider*>(b), info);
    }
    // Capsule-Sphere (ordered)
    if (ta == ColliderType::Capsule && tb == ColliderType::Sphere) {
        return TestCapsuleSphere(static_cast<CapsuleCollider*>(a),
                                static_cast<SphereCollider*>(b), info);
    }
    if (ta == ColliderType::Sphere && tb == ColliderType::Capsule) {
        bool result = TestCapsuleSphere(static_cast<CapsuleCollider*>(b),
                                        static_cast<SphereCollider*>(a), info);
        if (result) {
            std::swap(info.colliderA, info.colliderB);
            info.normal = info.normal * -1.0f;
        }
        return result;
    }
    // Capsule-Capsule
    if (ta == ColliderType::Capsule && tb == ColliderType::Capsule) {
        return TestCapsuleCapsule(static_cast<CapsuleCollider*>(a),
                                 static_cast<CapsuleCollider*>(b), info);
    }
    // Capsule-Box (ordered)
    if (ta == ColliderType::Capsule && tb == ColliderType::Box) {
        return TestCapsuleBox(static_cast<CapsuleCollider*>(a),
                             static_cast<BoxCollider*>(b), info);
    }
    if (ta == ColliderType::Box && tb == ColliderType::Capsule) {
        bool result = TestCapsuleBox(static_cast<CapsuleCollider*>(b),
                                     static_cast<BoxCollider*>(a), info);
        if (result) {
            std::swap(info.colliderA, info.colliderB);
            info.normal = info.normal * -1.0f;
        }
        return result;
    }

    // Unsupported collision type combination
    return false;
}

// === Raycasting ===

bool koilo::CollisionManager::Raycast(const Vector3D& origin, const Vector3D& direction,
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

int koilo::CollisionManager::RaycastAll(const Vector3D& origin, const Vector3D& direction,
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

bool koilo::CollisionManager::OverlapSphere(const Vector3D& center, float radius,
                                     uint32_t layerMask) {
    for (auto* collider : colliders) {
        if (!collider->IsEnabled()) continue;
        if (!IsLayerInMask(collider->GetLayer(), layerMask)) continue;

        Vector3D closest = collider->ClosestPoint(center);
        float dist = (closest - center).Magnitude();
        if (dist <= radius) {
            return true;
        }
    }

    return false;
}

int koilo::CollisionManager::OverlapSphereAll(const Vector3D& center, float radius,
                                       std::vector<Collider*>& results,
                                       uint32_t layerMask) {
    results.clear();

    for (auto* collider : colliders) {
        if (!collider->IsEnabled()) continue;
        if (!IsLayerInMask(collider->GetLayer(), layerMask)) continue;

        Vector3D closest = collider->ClosestPoint(center);
        float dist = (closest - center).Magnitude();
        if (dist <= radius) {
            results.push_back(collider);
        }
    }

    return static_cast<int>(results.size());
}

bool koilo::CollisionManager::OverlapBox(const Vector3D& center, const Vector3D& extents,
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

void koilo::CollisionManager::AddCollisionEnterCallback(CollisionCallback callback) {
    onCollisionEnterCallbacks.push_back(callback);
}

void koilo::CollisionManager::AddCollisionStayCallback(CollisionCallback callback) {
    onCollisionStayCallbacks.push_back(callback);
}

void koilo::CollisionManager::AddCollisionExitCallback(CollisionCallback callback) {
    onCollisionExitCallbacks.push_back(callback);
}

void koilo::CollisionManager::ClearCallbacks() {
    onCollisionEnterCallbacks.clear();
    onCollisionStayCallbacks.clear();
    onCollisionExitCallbacks.clear();
}

// === Private Helper Methods ===

uint64_t koilo::CollisionManager::GetPairID(Collider* a, Collider* b) const {
    uintptr_t ptrA = reinterpret_cast<uintptr_t>(a);
    uintptr_t ptrB = reinterpret_cast<uintptr_t>(b);

    // Ensure consistent ordering
    if (ptrA > ptrB) std::swap(ptrA, ptrB);

    return (static_cast<uint64_t>(ptrA) << 32) | static_cast<uint64_t>(ptrB);
}

bool koilo::CollisionManager::IsLayerInMask(int layer, uint32_t mask) const {
    return (mask & (1 << layer)) != 0;
}

bool koilo::CollisionManager::TestSphereSphere(SphereCollider* a, SphereCollider* b,
                                       CollisionInfo& info) {
    Vector3D delta = b->GetPosition() - a->GetPosition();
    float dist = delta.Magnitude();
    float radiusSum = a->GetRadius() + b->GetRadius();

    if (dist < radiusSum) {
        info.penetrationDepth = radiusSum - dist;
        info.normal = delta.Normal();
        info.contactPoint = a->GetPosition() + info.normal * a->GetRadius();
        return true;
    }

    return false;
}

bool koilo::CollisionManager::TestSphereBox(SphereCollider* sphere, BoxCollider* box,
                                     CollisionInfo& info) {
    Vector3D closest = box->ClosestPoint(sphere->GetPosition());
    Vector3D delta = sphere->GetPosition() - closest;
    float dist = delta.Magnitude();

    if (dist < sphere->GetRadius()) {
        info.penetrationDepth = sphere->GetRadius() - dist;
        info.normal = delta.Normal();
        info.contactPoint = closest;
        return true;
    }

    return false;
}

bool koilo::CollisionManager::TestBoxBox(BoxCollider* a, BoxCollider* b, CollisionInfo& info) {
    // AABB-AABB with proper penetration depth (SAT on 3 axes)
    Vector3D minA = a->GetMinimum();
    Vector3D maxA = a->GetMaximum();
    Vector3D minB = b->GetMinimum();
    Vector3D maxB = b->GetMaximum();

    float overlapX = std::min(maxA.X, maxB.X) - std::max(minA.X, minB.X);
    float overlapY = std::min(maxA.Y, maxB.Y) - std::max(minA.Y, minB.Y);
    float overlapZ = std::min(maxA.Z, maxB.Z) - std::max(minA.Z, minB.Z);

    if (overlapX <= 0 || overlapY <= 0 || overlapZ <= 0) return false;

    // Find minimum overlap axis for contact normal
    Vector3D centerA = a->GetPosition();
    Vector3D centerB = b->GetPosition();
    Vector3D direction = centerB - centerA;

    if (overlapX <= overlapY && overlapX <= overlapZ) {
        info.penetrationDepth = overlapX;
        info.normal = Vector3D(direction.X >= 0 ? 1.0f : -1.0f, 0, 0);
    } else if (overlapY <= overlapZ) {
        info.penetrationDepth = overlapY;
        info.normal = Vector3D(0, direction.Y >= 0 ? 1.0f : -1.0f, 0);
    } else {
        info.penetrationDepth = overlapZ;
        info.normal = Vector3D(0, 0, direction.Z >= 0 ? 1.0f : -1.0f);
    }

    info.contactPoint = (centerA + centerB) * 0.5f;
    return true;
}

// === Capsule Collision Tests ===

Vector3D koilo::CollisionManager::ClosestPointOnSegment(const Vector3D& p,
                                                       const Vector3D& a,
                                                       const Vector3D& b) {
    Vector3D ab = b - a;
    float len2 = ab.DotProduct(ab);
    if (len2 < 1e-12f) return a;
    float t = (p - a).DotProduct(ab) / len2;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return a + ab * t;
}

bool koilo::CollisionManager::TestCapsuleSphere(CapsuleCollider* capsule, SphereCollider* sphere,
                                               CollisionInfo& info) {
    Vector3D p1, p2;
    capsule->GetSegment(p1, p2);

    Vector3D closest = ClosestPointOnSegment(sphere->GetPosition(), p1, p2);
    Vector3D delta = sphere->GetPosition() - closest;
    float dist = delta.Magnitude();
    float radiusSum = capsule->GetRadius() + sphere->GetRadius();

    if (dist >= radiusSum) return false;

    info.penetrationDepth = radiusSum - dist;
    info.normal = dist > 1e-6f ? delta * (1.0f / dist) : Vector3D(0, 1, 0);
    info.contactPoint = closest + info.normal * capsule->GetRadius();
    return true;
}

bool koilo::CollisionManager::TestCapsuleCapsule(CapsuleCollider* a, CapsuleCollider* b,
                                                CollisionInfo& info) {
    Vector3D a1, a2, b1, b2;
    a->GetSegment(a1, a2);
    b->GetSegment(b1, b2);

    // Find closest points between two line segments
    Vector3D d1 = a2 - a1;
    Vector3D d2 = b2 - b1;
    Vector3D r = a1 - b1;

    float aa = d1.DotProduct(d1);
    float ee = d2.DotProduct(d2);
    float ff = d2.DotProduct(r);

    float s, t;

    if (aa < 1e-6f && ee < 1e-6f) {
        s = t = 0.0f;
    } else if (aa < 1e-6f) {
        s = 0.0f;
        t = ff / ee;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
    } else {
        float cc = d1.DotProduct(r);
        if (ee < 1e-6f) {
            t = 0.0f;
            s = -cc / aa;
            if (s < 0.0f) s = 0.0f;
            if (s > 1.0f) s = 1.0f;
        } else {
            float bb = d1.DotProduct(d2);
            float denom = aa * ee - bb * bb;
            if (denom != 0.0f) {
                s = (bb * ff - cc * ee) / denom;
                if (s < 0.0f) s = 0.0f;
                if (s > 1.0f) s = 1.0f;
            } else {
                s = 0.0f;
            }
            t = (bb * s + ff) / ee;
            if (t < 0.0f) {
                t = 0.0f;
                s = -cc / aa;
                if (s < 0.0f) s = 0.0f;
                if (s > 1.0f) s = 1.0f;
            } else if (t > 1.0f) {
                t = 1.0f;
                s = (bb - cc) / aa;
                if (s < 0.0f) s = 0.0f;
                if (s > 1.0f) s = 1.0f;
            }
        }
    }

    Vector3D closestA = a1 + d1 * s;
    Vector3D closestB = b1 + d2 * t;
    Vector3D delta = closestA - closestB;
    float dist = delta.Magnitude();
    float radiusSum = a->GetRadius() + b->GetRadius();

    if (dist >= radiusSum) return false;

    info.penetrationDepth = radiusSum - dist;
    info.normal = dist > 1e-6f ? delta * (1.0f / dist) : Vector3D(0, 1, 0);
    info.contactPoint = (closestA + closestB) * 0.5f;
    return true;
}

bool koilo::CollisionManager::TestCapsuleBox(CapsuleCollider* capsule, BoxCollider* box,
                                            CollisionInfo& info) {
    // Approximate: find closest point on capsule segment to box,
    // then treat as sphere-box with capsule radius
    Vector3D p1, p2;
    capsule->GetSegment(p1, p2);

    // Find the point on the segment closest to the box center
    Vector3D boxCenter = box->GetPosition();
    Vector3D segPoint = ClosestPointOnSegment(boxCenter, p1, p2);

    // Find closest point on box to that segment point
    Vector3D boxClosest = box->ClosestPoint(segPoint);

    Vector3D delta = segPoint - boxClosest;
    float dist = delta.Magnitude();

    if (dist >= capsule->GetRadius()) return false;

    info.penetrationDepth = capsule->GetRadius() - dist;
    info.normal = dist > 1e-6f ? delta * (1.0f / dist) : Vector3D(0, 1, 0);
    info.contactPoint = boxClosest;
    return true;
}

} // namespace koilo
