// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file convexhull.hpp
 * @brief Convex hull primitive: vertex list + support function for GJK/EPA.
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/geometry/3d/aabb.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <vector>
#include <array>
#include <cstddef>

namespace koilo {

/**
 * @class ConvexHull
 * @brief Convex hull defined by an explicit vertex list.
 *
 * `Support(dir)` returns the vertex with the maximum dot-product against
 * @p dir - the building block for GJK/EPA narrowphase.
 * Linear scan is intentional for now; a hill-climbing variant can replace
 * it later without changing the API.
 */
class ConvexHull {
public:
    std::vector<Vector3D> vertices;
    std::vector<std::array<int,3>> faces; ///< Optional: triangle indices into vertices.

    ConvexHull() = default;

    explicit ConvexHull(std::vector<Vector3D> verts) : vertices(std::move(verts)) {}

    void AddVertex(const Vector3D& v) { vertices.push_back(v); }
    void AddFace(int a, int b, int c) { faces.push_back({a,b,c}); }

    std::size_t VertexCount() const { return vertices.size(); }
    std::size_t FaceCount() const { return faces.size(); }

    /**
     * @brief Returns the vertex with the largest projection onto @p dir.
     */
    Vector3D Support(const Vector3D& dir) const {
        if (vertices.empty()) return Vector3D(0,0,0);
        std::size_t best = 0;
        float bestDot = vertices[0].DotProduct(dir);
        for (std::size_t i = 1; i < vertices.size(); ++i) {
            const float d = vertices[i].DotProduct(dir);
            if (d > bestDot) { bestDot = d; best = i; }
        }
        return vertices[best];
    }

    AABB ComputeBounds() const {
        if (vertices.empty()) return AABB(Vector3D(0,0,0), Vector3D(0,0,0));
        Vector3D mn = vertices[0], mx = vertices[0];
        for (std::size_t i = 1; i < vertices.size(); ++i) {
            const Vector3D& v = vertices[i];
            if (v.X < mn.X) { mn.X = v.X; } else if (v.X > mx.X) { mx.X = v.X; }
            if (v.Y < mn.Y) { mn.Y = v.Y; } else if (v.Y > mx.Y) { mx.Y = v.Y; }
            if (v.Z < mn.Z) { mn.Z = v.Z; } else if (v.Z > mx.Z) { mx.Z = v.Z; }
        }
        return AABB(mn, mx);
    }

    KL_BEGIN_FIELDS(ConvexHull)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ConvexHull)
        KL_METHOD_AUTO(ConvexHull, Support, "Support"),
        KL_METHOD_AUTO(ConvexHull, ComputeBounds, "Compute bounds"),
        KL_METHOD_AUTO(ConvexHull, VertexCount, "Vertex count"),
        KL_METHOD_AUTO(ConvexHull, FaceCount, "Face count")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ConvexHull)
        KL_CTOR0(ConvexHull)
    KL_END_DESCRIBE(ConvexHull)
};

} // namespace koilo
