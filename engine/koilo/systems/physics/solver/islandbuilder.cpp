// SPDX-License-Identifier: GPL-3.0-or-later
#include "islandbuilder.hpp"

#include "../rigidbody.hpp"
#include "../colliderproxy.hpp"
#include "../joints/joint.hpp"

#include <algorithm>
#include <unordered_map>

namespace koilo {

namespace {

struct UnionFind {
    std::vector<std::uint32_t> parent;
    std::vector<std::uint32_t> rank;

    void Init(std::size_t n) {
        parent.resize(n);
        rank.assign(n, 0);
        for (std::size_t i = 0; i < n; ++i) parent[i] = static_cast<std::uint32_t>(i);
    }

    std::uint32_t Find(std::uint32_t x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]]; // path compression (one-pass)
            x = parent[x];
        }
        return x;
    }

    // Union-by-lowest-id for determinism (canonical root = min body id).
    void Union(std::uint32_t a, std::uint32_t b) {
        std::uint32_t ra = Find(a), rb = Find(b);
        if (ra == rb) return;
        if (ra < rb) parent[rb] = ra; else parent[ra] = rb;
    }
};

// "Active" means: not null, not static. Optionally treats sleeping as active.
inline bool IsActive(const RigidBody* rb, bool treatSleepingAsBridge) {
    if (!rb) return false;
    if (rb->IsStatic()) return false;
    if (rb->IsSleeping() && !treatSleepingAsBridge) return false;
    return true;
}

inline std::uint32_t BodyId(const ColliderProxy* p) {
    if (!p) return ColliderProxy::kInvalidHandle;
    return p->bodyId;
}

} // namespace

std::vector<Island> IslandBuilder::Build(const std::vector<ContactManifold>& manifolds,
                                         const std::vector<RigidBody*>& bodies,
                                         bool treatSleepingAsBridge) {
    static const std::vector<Joint*> kNoJoints;
    return Build(manifolds, kNoJoints, bodies, treatSleepingAsBridge);
}

std::vector<Island> IslandBuilder::Build(const std::vector<ContactManifold>& manifolds,
                                         const std::vector<Joint*>& joints,
                                         const std::vector<RigidBody*>& bodies,
                                         bool treatSleepingAsBridge) {
    UnionFind uf;
    uf.Init(bodies.size());

    // Edge pass: union bodies sharing a manifold (only if both are "active").
    for (const ContactManifold& m : manifolds) {
        const std::uint32_t ia = BodyId(m.a);
        const std::uint32_t ib = BodyId(m.b);
        if (ia >= bodies.size() || ib >= bodies.size()) continue;
        if (ia == ib) continue;
        if (!IsActive(bodies[ia], treatSleepingAsBridge)) continue;
        if (!IsActive(bodies[ib], treatSleepingAsBridge)) continue;
        uf.Union(ia, ib);
    }

    // Joint edge pass: need RigidBody* to body-index lookup since
    // joints don't have ColliderProxy. Built lazily - only if joints exist.
    if (!joints.empty()) {
        std::unordered_map<const RigidBody*, std::uint32_t> ptrToIdx;
        ptrToIdx.reserve(bodies.size());
        for (std::uint32_t i = 0; i < bodies.size(); ++i) {
            if (bodies[i]) ptrToIdx.emplace(bodies[i], i);
        }
        auto resolve = [&](const RigidBody* rb) -> std::uint32_t {
            if (!rb) return ColliderProxy::kInvalidHandle;
            auto it = ptrToIdx.find(rb);
            return (it != ptrToIdx.end()) ? it->second : ColliderProxy::kInvalidHandle;
        };
        for (const Joint* j : joints) {
            if (!j) continue;
            const std::uint32_t ia = resolve(j->GetBodyA());
            const std::uint32_t ib = resolve(j->GetBodyB());
            if (ia >= bodies.size() || ib >= bodies.size()) continue;
            if (ia == ib) continue;
            if (!IsActive(bodies[ia], treatSleepingAsBridge)) continue;
            if (!IsActive(bodies[ib], treatSleepingAsBridge)) continue;
            uf.Union(ia, ib);
        }
    }

    // Bucket bodies by root, but only include "real" dynamic-or-sleeping bodies
    // (skip null/static - they're per-edge anchors, never island members).
    std::vector<Island> islands;
    std::vector<std::uint32_t> rootToIsland(bodies.size(), ColliderProxy::kInvalidHandle);

    for (std::uint32_t i = 0; i < bodies.size(); ++i) {
        const RigidBody* rb = bodies[i];
        if (!rb) continue;
        if (rb->IsStatic()) continue;
        // Sleeping bodies are island members so the SleepManager can decide to
        // keep them asleep (or wake them) based on the island's awake bodies.
        const std::uint32_t r = uf.Find(i);
        std::uint32_t islandIdx = rootToIsland[r];
        if (islandIdx == ColliderProxy::kInvalidHandle) {
            islandIdx = static_cast<std::uint32_t>(islands.size());
            rootToIsland[r] = islandIdx;
            Island isl;
            isl.canonicalId = r;          // root == min body id thanks to union-by-lowest-id
            islands.push_back(std::move(isl));
        }
        islands[islandIdx].bodyIds.push_back(i);
    }

    // Attach manifold indices to their island (look up via either body's root).
    for (std::uint32_t mi = 0; mi < manifolds.size(); ++mi) {
        const ContactManifold& m = manifolds[mi];
        const std::uint32_t ia = BodyId(m.a);
        const std::uint32_t ib = BodyId(m.b);
        std::uint32_t pick = ColliderProxy::kInvalidHandle;
        if (ia < bodies.size() && bodies[ia] && !bodies[ia]->IsStatic()) pick = ia;
        else if (ib < bodies.size() && bodies[ib] && !bodies[ib]->IsStatic()) pick = ib;
        if (pick == ColliderProxy::kInvalidHandle) continue;
        const std::uint32_t r = uf.Find(pick);
        const std::uint32_t islandIdx = rootToIsland[r];
        if (islandIdx == ColliderProxy::kInvalidHandle) continue;
        islands[islandIdx].manifoldIdx.push_back(mi);
    }

    // Determinism: islands sorted by canonicalId; bodyIds + manifoldIdx are
    // already in ascending order because we walked the input in order.
    std::sort(islands.begin(), islands.end(),
              [](const Island& a, const Island& b) { return a.canonicalId < b.canonicalId; });

    return islands;
}

} // namespace koilo
