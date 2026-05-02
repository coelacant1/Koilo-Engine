// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file mesh_bvh.hpp
 * @brief Compact AABB BVH for accelerating ray-mesh intersection tests.
 *
 * Built lazily by MeshRaycast::Raycast and cached on the Mesh; rebuilt only
 * when the mesh's gpuVersion changes. Reduces a per-ray O(N) triangle scan
 * to O(log N) for typical meshes.
 *
 * @author Coela
 */

#pragma once

#include <cstdint>
#include <vector>

#include <koilo/core/geometry/ray.hpp>
#include <koilo/core/math/vector3d.hpp>

namespace koilo {

class IndexGroup;

class MeshBVH {
public:
    struct Node {
        Vector3D bmin;
        Vector3D bmax;
        // Interior node: left = first child index, count = 0
        // Leaf node:    left = first triangle index in tri_, count > 0
        uint32_t left  = 0;
        uint32_t count = 0;
    };

    MeshBVH() = default;

    /// Build the BVH from a triangle mesh in world space.
    /// vertices/indices/triCount must outlive any Intersect() call until rebuild.
    void Build(const Vector3D* vertices,
               const IndexGroup* indices,
               int triCount);

    /// Returns true if the ray hits any triangle within maxDistance. The
    /// closest hit is returned via outDistance/outPoint/outNormal. Triangle
    /// vertices are resolved on demand from the same vertices/indices used
    /// at Build() time (stored as bare pointers - caller must keep them alive).
    bool Intersect(const Ray& ray,
                   float maxDistance,
                   bool backfaceCulling,
                   float& outDistance,
                   Vector3D& outPoint,
                   Vector3D& outNormal) const;

    bool Empty() const { return nodes_.empty(); }

private:
    void Subdivide(uint32_t nodeIdx,
                   const Vector3D* vertices,
                   const IndexGroup* indices,
                   int depth);

    std::vector<Node>     nodes_;
    std::vector<uint32_t> tri_;        // triangle indices reordered by build
    std::vector<Vector3D> centroids_;  // per-original-triangle centroid (build-time scratch, kept for rebuilds)
    const Vector3D* sourceVertices_ = nullptr;
    const IndexGroup* sourceIndices_ = nullptr;
};

} // namespace koilo
