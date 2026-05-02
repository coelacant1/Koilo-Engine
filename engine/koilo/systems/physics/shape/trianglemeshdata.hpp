// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file trianglemeshdata.hpp
 * @brief Shared static-mesh geometry + BVH.
 *
 * Held by `std::shared_ptr<const TriangleMeshData>` so multiple MeshColliders
 * (e.g. instanced level geometry) can share one BVH. Build() must be called
 * after vertices/indices are populated and before the data is first queried.
 *
 * Mesh frame is "shape local". When attached to a MeshCollider, the collider's
 * body pose transforms the mesh into world space. Sphere-vs-mesh narrowphase
 * brings the dynamic side into mesh-local for query, which is far cheaper than
 * transforming all candidate triangles.
 */

#pragma once

#include <koilo/core/geometry/3d/aabb.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/systems/physics/shape/meshbvh.hpp>

#include <cstdint>
#include <vector>

namespace koilo {

struct TriangleMeshData {
    std::vector<Vector3D>     vertices;
    std::vector<std::uint32_t> indices;     ///< 3 per triangle.
    AABB                      localAabb;
    TriangleMeshBVH                   bvh;

    /**
     * @brief Recomputes localAabb and rebuilds the BVH from current vertices/indices.
     * Degenerate triangles (zero-area or out-of-range index) are filtered out.
     */
    void Build(std::uint32_t leafSize = 4) {
        if (vertices.empty()) {
            localAabb = AABB();
            bvh.Build({}, {});
            return;
        }
        localAabb = AABB(vertices[0], vertices[0]);
        for (std::size_t i = 1; i < vertices.size(); ++i) {
            localAabb.Encapsulate(vertices[i]);
        }
        bvh.Build(vertices, indices, leafSize);
    }

    bool Empty() const { return bvh.Empty(); }
};

} // namespace koilo
