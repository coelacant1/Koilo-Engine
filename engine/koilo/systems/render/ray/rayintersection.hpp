// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file rayintersection.hpp
 * @brief Ray-scene intersection utilities for ray trace rendering.
 *
 * Provides intersection testing and shading calculations for ray traced rendering.
 * Works with the Scene and Camera systems to generate rendered images.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <koilo/core/geometry/ray.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/color/color888.hpp>
#include <koilo/systems/scene/scene.hpp>
#include <koilo/systems/scene/mesh.hpp>
#include <koilo/systems/render/material/imaterial.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct RayHitInfo
 * @brief Contains detailed information about a ray-scene intersection for rendering.
 */
struct RayHitInfo {
    bool hit;                ///< True if ray hit something
    float distance;          ///< Distance from ray origin to hit point
    Vector3D point;          ///< Hit point in world space
    Vector3D normal;         ///< Surface normal at hit point
    Color888 color;          ///< Base color at hit point
    Mesh* mesh;              ///< The mesh that was hit (if any)
    IMaterial* material;     ///< Material at hit point (if any)

    RayHitInfo()
        : hit(false), distance(0.0f), point(0, 0, 0), normal(0, 1, 0),
          color(0, 0, 0), mesh(nullptr), material(nullptr) {}

    KL_BEGIN_FIELDS(RayHitInfo)
        KL_FIELD(RayHitInfo, hit, "Hit", 0, 1),
        KL_FIELD(RayHitInfo, distance, "Distance", 0, 0),
        KL_FIELD(RayHitInfo, point, "Point", 0, 0),
        KL_FIELD(RayHitInfo, normal, "Normal", 0, 0),
        KL_FIELD(RayHitInfo, color, "Color", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(RayHitInfo)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(RayHitInfo)
        KL_CTOR0(RayHitInfo)
    KL_END_DESCRIBE(RayHitInfo)
};

/**
 * @class RayIntersection
 * @brief Static utility class for ray-scene intersection and lighting calculations.
 */
class RayIntersection {
public:
    /**
     * @brief Tests ray against entire scene, finding closest intersection.
     * @param ray The ray to cast.
     * @param scene Pointer to the scene to test against.
     * @return Hit information (hit=false if no intersection).
     */
    static RayHitInfo IntersectScene(const Ray& ray, Scene* scene);

    /**
     * @brief Tests ray against a single mesh.
     * @param ray The ray to cast.
     * @param mesh Pointer to the mesh to test against.
     * @return Hit information (hit=false if no intersection).
     */
    static RayHitInfo IntersectMesh(const Ray& ray, Mesh* mesh);

    /**
     * @brief Calculates lighting at a hit point.
     * @param hit Hit information.
     * @param scene Scene containing lights.
     * @param incomingRay The ray that hit the surface.
     * @param ambientLight Ambient light intensity (0-1).
     * @return Final shaded color.
     */
    static Color888 CalculateLighting(const RayHitInfo& hit,
                                     Scene* scene,
                                     const Ray& incomingRay,
                                     float ambientLight = 0.1f);

    /**
     * @brief Checks if a point can see a light (for shadow calculation).
     * @param point Point to test from.
     * @param lightPosition Light position.
     * @param scene Scene containing occluding objects.
     * @return True if point has line-of-sight to light.
     */
    static bool IsLightVisible(const Vector3D& point,
                              const Vector3D& lightPosition,
                              Scene* scene);

private:
    KL_BEGIN_FIELDS(RayIntersection)
        /* No reflected fields - static utility class. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(RayIntersection)
        KL_SMETHOD_AUTO(RayIntersection::IntersectScene, "Intersect scene"),
        KL_SMETHOD_AUTO(RayIntersection::IntersectMesh, "Intersect mesh"),
        KL_SMETHOD_AUTO(RayIntersection::CalculateLighting, "Calculate lighting"),
        KL_SMETHOD_AUTO(RayIntersection::IsLightVisible, "Is light visible")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(RayIntersection)
        /* No reflected ctors - static utility class. */
    KL_END_DESCRIBE(RayIntersection)
};

} // namespace koilo
