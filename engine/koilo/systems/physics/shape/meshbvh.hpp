// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file meshbvh.hpp
 * @brief Top-down median-split BVH over a triangle list.
 *
 * Static-only. Owns a remap table so the source vertex/index
 * buffers stay untouched and can be shared (TriangleMeshData uses this).
 *
 * Build is deterministic: nth_element on centroid axis with original triangle
 * index as the tie-break (Determinism Tier 0/1).
 *
 * Query is iterative (explicit stack) to bound stack depth on deep meshes.
 */

#pragma once

#include <koilo/core/geometry/3d/aabb.hpp>
#include <koilo/core/math/vector3d.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

namespace koilo {

class TriangleMeshBVH {
public:
    struct Node {
        AABB     bounds;
        std::int32_t  left      = -1;     ///< Left child index, or -1 for a leaf.
        std::int32_t  right     = -1;     ///< Right child index, or -1 for a leaf.
        std::uint32_t firstTri  = 0;      ///< Index into triRemap_ (leaves only).
        std::uint32_t triCount  = 0;      ///< Number of triangles in this leaf.
    };

    /**
     * @brief Builds the BVH from a vertex/index pair (triangle list, 3*N indices).
     * @param vertices Vertex positions.
     * @param indices  Triangle indices (3 per triangle). Caller's storage; not retained.
     * @param leafSize Target max triangles per leaf.
     */
    void Build(const std::vector<Vector3D>& vertices,
               const std::vector<std::uint32_t>& indices,
               std::uint32_t leafSize = 4);

    /** Total bounds of the mesh. */
    AABB Bounds() const { return nodes_.empty() ? AABB() : nodes_[0].bounds; }

    /** Number of triangles indexed by this BVH (after degenerate filtering). */
    std::uint32_t TriangleCount() const { return static_cast<std::uint32_t>(triRemap_.size()); }

    bool Empty() const { return nodes_.empty(); }

    /**
     * @brief DFS query: invokes cb(triIndex) for each triangle whose AABB overlaps q.
     * triIndex is the original index into the source index buffer / 3
     * (i.e. the triangle's position in the input list).
     */
    template <class Fn>
    void Query(const AABB& q, Fn cb) const {
        if (nodes_.empty()) return;
        if (!nodes_[0].bounds.Overlaps(q)) return;

        std::int32_t stack[64];
        std::int32_t sp = 0;
        stack[sp++] = 0;

        while (sp > 0) {
            const std::int32_t ni = stack[--sp];
            const Node& n = nodes_[ni];
            if (!n.bounds.Overlaps(q)) continue;

            if (n.triCount > 0) {
                const std::uint32_t end = n.firstTri + n.triCount;
                for (std::uint32_t i = n.firstTri; i < end; ++i) {
                    cb(triRemap_[i]);
                }
            } else {
                if (n.left  >= 0 && sp < 64) stack[sp++] = n.left;
                if (n.right >= 0 && sp < 64) stack[sp++] = n.right;
            }
        }
    }

    /**
     * @brief Ray-vs-BVH traversal. Returns true if any leaf triangle satisfies
     * the user predicate; visit order is bounds-front-to-back to allow early-out.
     * cb is invoked as `cb(triIndex, tNear) -> bool`. Returning true from cb
     * tightens the maximum traversal distance to the current best `tBest`,
     * pruning farther sub-trees. tBest must be updated by the caller via the
     * same closure (typical pattern: capture a float& and clamp on hit).
     *
     * `tMax` bounds traversal distance; pass FLT_MAX for unbounded.
     */
    template <class Fn>
    void Raycast(const Vector3D& origin, const Vector3D& dir, float tMax, Fn cb) const {
        if (nodes_.empty()) return;

        // Slab test against AABB. Returns tNear; -1 if miss / behind / past tMax.
        auto rayAabbT = [&](const AABB& box, float curMax) -> float {
            float tmin = 0.0f, tmax = curMax;
            for (int axis = 0; axis < 3; ++axis) {
                const float o = (axis == 0) ? origin.X : (axis == 1) ? origin.Y : origin.Z;
                const float d = (axis == 0) ? dir.X    : (axis == 1) ? dir.Y    : dir.Z;
                const float lo = (axis == 0) ? box.min.X : (axis == 1) ? box.min.Y : box.min.Z;
                const float hi = (axis == 0) ? box.max.X : (axis == 1) ? box.max.Y : box.max.Z;
                if (std::fabs(d) < 1e-8f) {
                    if (o < lo || o > hi) return -1.0f;
                } else {
                    const float invD = 1.0f / d;
                    float t1 = (lo - o) * invD;
                    float t2 = (hi - o) * invD;
                    if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
                    if (t1 > tmin) tmin = t1;
                    if (t2 < tmax) tmax = t2;
                    if (tmin > tmax) return -1.0f;
                }
            }
            return tmin;
        };

        struct Entry { std::int32_t node; float tNear; };
        Entry stack[64];
        std::int32_t sp = 0;

        const float t0 = rayAabbT(nodes_[0].bounds, tMax);
        if (t0 < 0.0f) return;
        stack[sp++] = { 0, t0 };

        float bestT = tMax;

        while (sp > 0) {
            const Entry e = stack[--sp];
            if (e.tNear > bestT) continue;
            const Node& n = nodes_[e.node];
            if (n.triCount > 0) {
                const std::uint32_t end = n.firstTri + n.triCount;
                for (std::uint32_t i = n.firstTri; i < end; ++i) {
                    if (cb(triRemap_[i], bestT)) {
                        // cb is expected to clamp bestT externally on hit.
                    }
                }
            } else {
                std::int32_t cl = n.left, cr = n.right;
                float tl = (cl >= 0) ? rayAabbT(nodes_[cl].bounds, bestT) : -1.0f;
                float tr = (cr >= 0) ? rayAabbT(nodes_[cr].bounds, bestT) : -1.0f;
                // Push farther child first so the nearer is visited next.
                if (tl >= 0.0f && tr >= 0.0f) {
                    if (tl < tr) {
                        if (sp < 64) stack[sp++] = { cr, tr };
                        if (sp < 64) stack[sp++] = { cl, tl };
                    } else {
                        if (sp < 64) stack[sp++] = { cl, tl };
                        if (sp < 64) stack[sp++] = { cr, tr };
                    }
                } else if (tl >= 0.0f) {
                    if (sp < 64) stack[sp++] = { cl, tl };
                } else if (tr >= 0.0f) {
                    if (sp < 64) stack[sp++] = { cr, tr };
                }
            }
        }
    }

    const std::vector<Node>& Nodes()    const { return nodes_; }
    const std::vector<std::uint32_t>& Remap() const { return triRemap_; }

private:
    std::vector<Node>          nodes_;
    std::vector<std::uint32_t> triRemap_;   ///< triRemap_[i] = original triangle index.

    std::int32_t BuildRecursive(std::vector<AABB>& triBounds,
                                std::vector<Vector3D>& triCentroids,
                                std::uint32_t first, std::uint32_t count,
                                std::uint32_t leafSize, int depth);
};

} // namespace koilo
