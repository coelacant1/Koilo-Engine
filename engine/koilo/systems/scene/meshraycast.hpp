// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file meshraycast.hpp
 * @brief Raycasting against mesh geometry for physics and rendering.
 *
 * Provides triangle-mesh intersection testing using the Möller-Trumbore algorithm.
 * Used for both physics raycasting against mesh colliders and rendering ray tracing.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <koilo/core/geometry/ray.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/systems/physics/raycasthit.hpp>
#include "mesh.hpp"
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class MeshRaycast
 * @brief Static utility class for ray-mesh intersection testing.
 *
 * Implements the Möller-Trumbore ray-triangle intersection algorithm
 * for fast, efficient mesh raycasting.
 */
class MeshRaycast {
public:
    /**
     * @brief Performs raycast against a mesh's triangles.
     * @param ray The ray to cast.
     * @param mesh Pointer to the mesh to test against.
     * @param hit Output hit information.
     * @param maxDistance Maximum ray distance.
     * @param backfaceCulling If true, ignores backfacing triangles.
     * @return True if ray hits any triangle in the mesh.
     */
    static bool Raycast(const Ray& ray,
                       Mesh* mesh,
                       RaycastHit& hit,
                       float maxDistance,
                       bool backfaceCulling = true);

    /**
     * @brief Performs raycast against a single triangle (Möller-Trumbore algorithm).
     * @param ray The ray to cast.
     * @param v0 First vertex of triangle.
     * @param v1 Second vertex of triangle.
     * @param v2 Third vertex of triangle.
     * @param distance Output distance to hit point (if hit).
     * @param hitPoint Output hit point in world space (if hit).
     * @param normal Output surface normal at hit point (if hit).
     * @param backfaceCulling If true, ignores backfacing triangles.
     * @return True if ray intersects triangle.
     */
    static bool RaycastTriangle(const Ray& ray,
                               const Vector3D& v0,
                               const Vector3D& v1,
                               const Vector3D& v2,
                               float& distance,
                               Vector3D& hitPoint,
                               Vector3D& normal,
                               bool backfaceCulling = true);

    /**
     * @brief Computes barycentric coordinates for a point in a triangle.
     * @param p Point to test.
     * @param a First vertex.
     * @param b Second vertex.
     * @param c Third vertex.
     * @param u Output barycentric coordinate u.
     * @param v Output barycentric coordinate v.
     * @param w Output barycentric coordinate w (where u+v+w=1).
     */
    static void ComputeBarycentric(const Vector3D& p,
                                   const Vector3D& a,
                                   const Vector3D& b,
                                   const Vector3D& c,
                                   float& u, float& v, float& w);

private:
    static constexpr float EPSILON = 0.0000001f;  ///< Epsilon for float comparisons

    KL_BEGIN_FIELDS(MeshRaycast)
        /* No reflected fields - static utility class. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(MeshRaycast)
        KL_SMETHOD_AUTO(MeshRaycast::Raycast, "Raycast"),
        KL_SMETHOD_AUTO(MeshRaycast::RaycastTriangle, "Raycast triangle"),
        KL_SMETHOD_AUTO(MeshRaycast::ComputeBarycentric, "Compute barycentric")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MeshRaycast)
        /* No reflected ctors - static utility class. */
    KL_END_DESCRIBE(MeshRaycast)
};

} // namespace koilo
