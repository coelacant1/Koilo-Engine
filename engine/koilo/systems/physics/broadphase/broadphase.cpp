// SPDX-License-Identifier: GPL-3.0-or-later
#include "broadphase.hpp"
#include <algorithm>

namespace koilo {

void Broadphase::Add(ColliderProxy* proxy, const BodyPose& bodyPose, float margin) {
    if (!proxy) return;
    proxy->proxyId = nextProxyId_++;
    proxy->RefreshWorldAABB(bodyPose);
    if (IsPlanar(proxy)) {
        planes_.push_back(proxy);
        proxy->broadphaseHandle = ColliderProxy::kInvalidHandle;
    } else {
        const std::int32_t handle = tree_.Insert(proxy->worldAabb, proxy, margin);
        proxy->broadphaseHandle = static_cast<std::uint32_t>(handle);
    }
}

void Broadphase::Remove(ColliderProxy* proxy) {
    if (!proxy) return;
    if (IsPlanar(proxy)) {
        planes_.erase(std::remove(planes_.begin(), planes_.end(), proxy), planes_.end());
    } else if (proxy->broadphaseHandle != ColliderProxy::kInvalidHandle) {
        tree_.Remove(static_cast<std::int32_t>(proxy->broadphaseHandle));
        proxy->broadphaseHandle = ColliderProxy::kInvalidHandle;
    }
}

void Broadphase::Update(ColliderProxy* proxy, const BodyPose& bodyPose, float margin) {
    if (!proxy) return;
    proxy->RefreshWorldAABB(bodyPose);
    if (IsPlanar(proxy)) return; // no-op
    if (proxy->broadphaseHandle == ColliderProxy::kInvalidHandle) {
        Add(proxy, bodyPose, margin);
        return;
    }
    tree_.Move(static_cast<std::int32_t>(proxy->broadphaseHandle), proxy->worldAabb, margin);
}

void Broadphase::UpdateInflated(ColliderProxy* proxy, const BodyPose& bodyPose,
                                float inflateRadius, float leafMargin) {
    if (!proxy) return;
    proxy->RefreshWorldAABB(bodyPose);
    if (IsPlanar(proxy)) return;
    // Expand the worldAabb symmetrically. We deliberately mutate the proxy's
    // stored AABB so the narrowphase loop's bounding-radius derivation also
    // sees the swept extent (it picks up `worldAabb` directly).
    if (inflateRadius > 0.0f) {
        proxy->worldAabb.min.X -= inflateRadius;
        proxy->worldAabb.min.Y -= inflateRadius;
        proxy->worldAabb.min.Z -= inflateRadius;
        proxy->worldAabb.max.X += inflateRadius;
        proxy->worldAabb.max.Y += inflateRadius;
        proxy->worldAabb.max.Z += inflateRadius;
    }
    if (proxy->broadphaseHandle == ColliderProxy::kInvalidHandle) {
        const std::int32_t handle = tree_.Insert(proxy->worldAabb, proxy, leafMargin);
        proxy->broadphaseHandle = static_cast<std::uint32_t>(handle);
        return;
    }
    tree_.Move(static_cast<std::int32_t>(proxy->broadphaseHandle), proxy->worldAabb, leafMargin);
}

std::vector<std::pair<ColliderProxy*, ColliderProxy*>> Broadphase::CollectPairs() const {
    std::vector<std::pair<ColliderProxy*, ColliderProxy*>> out;

    // DBVT self-overlap pairs.
    std::vector<std::pair<std::int32_t, std::int32_t>> handlePairs;
    tree_.QueryAllPairs(handlePairs);
    out.reserve(handlePairs.size() + planes_.size() * 4);
    for (const auto& hp : handlePairs) {
        auto* a = static_cast<ColliderProxy*>(tree_.GetUserData(hp.first));
        auto* b = static_cast<ColliderProxy*>(tree_.GetUserData(hp.second));
        if (!a || !b) continue;
        if (a->proxyId < b->proxyId) out.emplace_back(a, b);
        else                          out.emplace_back(b, a);
    }

    // Plane ↔ tree-leaf pairs (always-test).
    for (auto* plane : planes_) {
        if (!plane || !plane->enabled) continue;
        // Walk every tree leaf; planes have infinite AABB so there's no point
        // in querying with it. We iterate the tree's leaves via a deep query
        // using a huge AABB.
        AABB huge(Vector3D(-1e30f, -1e30f, -1e30f), Vector3D(1e30f, 1e30f, 1e30f));
        tree_.Query(huge, [&](std::int32_t handle) {
            auto* other = static_cast<ColliderProxy*>(tree_.GetUserData(handle));
            if (!other || !other->enabled) return true;
            if (plane->proxyId < other->proxyId) out.emplace_back(plane, other);
            else                                  out.emplace_back(other, plane);
            return true;
        });
    }

    // Deterministic sort by (minProxyId, maxProxyId).
    std::sort(out.begin(), out.end(),
        [](const std::pair<ColliderProxy*, ColliderProxy*>& x,
           const std::pair<ColliderProxy*, ColliderProxy*>& y) {
            if (x.first->proxyId != y.first->proxyId)
                return x.first->proxyId < y.first->proxyId;
            return x.second->proxyId < y.second->proxyId;
        });
    return out;
}

} // namespace koilo
