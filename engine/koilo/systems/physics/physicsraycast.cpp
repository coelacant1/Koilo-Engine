// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/physics/physicsraycast.hpp>
#include <algorithm>

namespace koilo {

bool koilo::PhysicsRaycast::Raycast(const Ray& ray,
                             const std::vector<Collider*>& colliders,
                             RaycastHit& hit,
                             float maxDistance,
                             int layerMask) {
    bool hasHit = false;
    float closestDistance = maxDistance;
    RaycastHit closestHit;

    for (Collider* collider : colliders) {
        if (!collider || !collider->IsEnabled()) {
            continue;
        }

        // Check layer mask
        if (!LayerInMask(collider->GetLayer(), layerMask)) {
            continue;
        }

        RaycastHit tempHit;
        if (collider->Raycast(ray, tempHit, maxDistance)) {
            if (tempHit.distance < closestDistance) {
                closestDistance = tempHit.distance;
                closestHit = tempHit;
                hasHit = true;
            }
        }
    }

    if (hasHit) {
        hit = closestHit;
    }

    return hasHit;
}

std::vector<RaycastHit> koilo::PhysicsRaycast::RaycastAll(const Ray& ray,
                                                    const std::vector<Collider*>& colliders,
                                                    float maxDistance,
                                                    int layerMask) {
    std::vector<RaycastHit> hits;

    for (Collider* collider : colliders) {
        if (!collider || !collider->IsEnabled()) {
            continue;
        }

        // Check layer mask
        if (!LayerInMask(collider->GetLayer(), layerMask)) {
            continue;
        }

        RaycastHit hit;
        if (collider->Raycast(ray, hit, maxDistance)) {
            hits.push_back(hit);
        }
    }

    // Sort hits by distance (closest first)
    std::sort(hits.begin(), hits.end(),
              [](const RaycastHit& a, const RaycastHit& b) {
                  return a.distance < b.distance;
              });

    return hits;
}

bool koilo::PhysicsRaycast::RaycastAny(const Ray& ray,
                                const std::vector<Collider*>& colliders,
                                float maxDistance,
                                int layerMask) {
    for (Collider* collider : colliders) {
        if (!collider || !collider->IsEnabled()) {
            continue;
        }

        // Check layer mask
        if (!LayerInMask(collider->GetLayer(), layerMask)) {
            continue;
        }

        RaycastHit hit;
        if (collider->Raycast(ray, hit, maxDistance)) {
            return true;  // Early exit on first hit
        }
    }

    return false;
}

bool koilo::PhysicsRaycast::LayerInMask(int layer, int layerMask) {
    if (layerMask == -1) {
        return true;  // -1 means all layers
    }

    if (layer < 0 || layer > 31) {
        return false;  // Invalid layer
    }

    return (layerMask & (1 << layer)) != 0;
}

} // namespace koilo
