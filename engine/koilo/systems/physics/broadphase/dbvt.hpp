// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file dbvt.hpp
 * @brief Dynamic AABB tree (DBVT) for the broadphase.
 *
 * Box2D / btDbvt-style implementation. Each leaf wraps a fattened AABB
 * (`aabb` + margin) so small movements don't trigger reinsertion. Internal
 * nodes wrap the union of their children. Self-pair queries emit overlapping
 * leaf pairs; pair output is sorted by `(minProxyId, maxProxyId)` upstream
 * for T1 determinism.
 *
 * No tree rotations in v1. `Quality()` exposes height + area-ratio metrics
 * so a future hardening pass can introduce rebalancing if needed.
 *
 * Planes do **not** belong in the DBVT - their effectively infinite AABB
 * destroys query selectivity. Use `Broadphase` (which owns the DBVT plus
 * a separate plane registry) instead of touching this class directly.
 */

#pragma once

#include <koilo/core/geometry/3d/aabb.hpp>
#include <cstdint>
#include <vector>
#include <functional>

namespace koilo {

class DynamicAABBTree {
public:
    static constexpr std::int32_t kNullNode = -1;

    DynamicAABBTree() = default;

    /** Inserts a leaf wrapping `aabb` + `margin`. Returns the node handle. */
    std::int32_t Insert(const AABB& aabb, void* userData, float margin = 0.05f);

    /** Removes a leaf. Subsequent calls with the handle are undefined. */
    void Remove(std::int32_t handle);

    /**
     * @brief Updates a leaf's AABB. If the new (un-fattened) AABB still fits
     * inside the stored fat AABB, this is a no-op and returns false.
     * Otherwise the leaf is reinserted with a fresh fat AABB and returns true.
     */
    bool Move(std::int32_t handle, const AABB& aabb, float margin = 0.05f);

    /** Returns the user data attached to a leaf. */
    void* GetUserData(std::int32_t handle) const { return nodes_[handle].userData; }

    /** Returns the (fattened) AABB of a leaf. */
    const AABB& GetFatAABB(std::int32_t handle) const { return nodes_[handle].aabb; }

    /** Visits every leaf whose fat AABB overlaps `aabb`. Callback returns false to stop. */
    void Query(const AABB& aabb, const std::function<bool(std::int32_t)>& cb) const;

    /**
     * @brief Self-overlap query producing every leaf pair (a,b) with a < b
     * (handle order). Output is appended; caller is expected to map handles
     * to proxyIds and sort `(minProxyId, maxProxyId)` for determinism.
     */
    void QueryAllPairs(std::vector<std::pair<std::int32_t, std::int32_t>>& out) const;

    /** Diagnostics. */
    struct Quality {
        std::int32_t nodeCount   = 0;
        std::int32_t leafCount   = 0;
        std::int32_t height      = 0;
        float        areaRatio   = 0.0f;   ///< total internal area / root area
    };
    Quality ComputeQuality() const;

    std::int32_t Root() const { return root_; }
    std::size_t  NodeCount() const { return nodes_.size() - freeList_.size(); }

private:
    struct Node {
        AABB     aabb;
        void*    userData = nullptr;
        std::int32_t parent = kNullNode;
        std::int32_t child1 = kNullNode;
        std::int32_t child2 = kNullNode;
        std::int32_t height = -1;
        bool IsLeaf() const { return child1 == kNullNode; }
    };

    std::vector<Node> nodes_;
    std::vector<std::int32_t> freeList_;
    std::int32_t root_ = kNullNode;

    std::int32_t AllocateNode();
    void FreeNode(std::int32_t handle);
    void InsertLeaf(std::int32_t leaf);
    void RemoveLeaf(std::int32_t leaf);
    static AABB Union(const AABB& a, const AABB& b);
    static AABB Inflate(const AABB& a, float margin);
    static float Area(const AABB& a);
    static bool Contains(const AABB& outer, const AABB& inner);

    void DescendPair(std::int32_t a, std::int32_t b,
                     std::vector<std::pair<std::int32_t, std::int32_t>>& out) const;
    void DescendSelf(std::int32_t a,
                     std::vector<std::pair<std::int32_t, std::int32_t>>& out) const;
};

} // namespace koilo
