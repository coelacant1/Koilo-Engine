// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file broadphase.hpp
 * @brief Broadphase facade - DBVT for finite shapes + plane registry for
 * effectively-infinite shapes.
 *
 * Each registered ColliderProxy gets a stable `proxyId` (assigned in the
 * order proxies were added). Pair output is sorted lexicographically by
 * `(minProxyId, maxProxyId)` for T1 determinism and matches the
 * `ContactCache` key canonicalization.
 *
 * Planes are kept out of the DBVT (their AABB is essentially infinite and
 * would destroy query selectivity). They are tested every frame against
 * each registered finite proxy.
 */

#pragma once

#include "dbvt.hpp"
#include <koilo/systems/physics/colliderproxy.hpp>
#include <koilo/systems/physics/shape/ishape.hpp>
#include <cstdint>
#include <vector>
#include <utility>

namespace koilo {

class Broadphase {
public:
    /** Inserts a proxy. Assigns a fresh `proxyId` and routes it to the
     *  correct backing structure (DBVT or plane list) based on shape type. */
    void Add(ColliderProxy* proxy, const BodyPose& bodyPose, float margin = 0.05f);

    /** Removes a proxy. */
    void Remove(ColliderProxy* proxy);

    /** Refreshes a proxy's worldAabb from `bodyPose` and updates the tree. */
    void Update(ColliderProxy* proxy, const BodyPose& bodyPose, float margin = 0.05f);

    /**
     * @brief CCD: like `Update`, but expands the proxy's worldAabb
     * symmetrically by `inflateRadius` on every axis BEFORE re-fitting the
     * DBVT leaf. Used for `bullet`-flagged bodies so the next broadphase pair
     * collection finds candidates the body would otherwise tunnel past in a
     * single fixed step. The `leafMargin` is the standard DBVT fattening
     * applied on top (defaults to the same 0.05 used by `Update`).
     */
    void UpdateInflated(ColliderProxy* proxy, const BodyPose& bodyPose,
                        float inflateRadius, float leafMargin = 0.05f);

    /**
     * @brief Returns the deterministic sorted list of overlapping pairs.
     * Includes both DBVT self-pairs and plane↔dbvt pairs. Each pair has
     * `first.proxyId < second.proxyId`.
     */
    std::vector<std::pair<ColliderProxy*, ColliderProxy*>> CollectPairs() const;

    /** Diagnostics passthrough. */
    DynamicAABBTree::Quality Quality() const { return tree_.ComputeQuality(); }
    std::size_t TreeNodeCount() const { return tree_.NodeCount(); }
    std::size_t PlaneCount() const { return planes_.size(); }

private:
    DynamicAABBTree tree_;
    std::vector<ColliderProxy*> planes_;          ///< Effectively-infinite shapes (Plane).
    std::vector<ColliderProxy*> dbvtProxies_;     ///< For mapping handle -> proxy in pair output.
    std::uint32_t nextProxyId_ = 0;

    static bool IsPlanar(const ColliderProxy* p) {
        return p && p->shape && p->shape->Type() == ShapeType::Plane;
    }
};

} // namespace koilo
