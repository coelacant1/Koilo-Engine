// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ray.hpp
 * @brief Ray class for both physics raycasting and ray trace rendering.
 *
 * Unified ray representation used by both the physics system (collision detection)
 * and the rendering system (ray tracing). This provides a single, type-safe
 * interface for all ray-based operations.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class Ray
 * @brief Represents a ray with an origin point and direction vector.
 *
 * A ray is defined as: P(t) = origin + t * direction, where t >= 0.
 * The direction vector should be normalized for consistent distance calculations.
 *
 * This class is used for:
 * - Physics raycasting (collision detection)
 * - Ray trace rendering (image synthesis)
 * - Picking and selection
 * - Line-of-sight calculations
 */
class Ray {
public:
    Vector3D origin;     ///< Ray origin point in world space
    Vector3D direction;  ///< Ray direction (should be normalized)

    /**
     * @brief Default constructor.
     * Creates a ray at origin pointing along +Z axis.
     */
    Ray();

    /**
     * @brief Constructor with origin and direction.
     * @param origin Ray origin point.
     * @param direction Ray direction (will be normalized if not already).
     */
    Ray(const Vector3D& origin, const Vector3D& direction);

    /**
     * @brief Copy constructor.
     */
    Ray(const Ray& other) = default;

    /**
     * @brief Assignment operator.
     */
    Ray& operator=(const Ray& other) = default;

    /**
     * @brief Gets a point along the ray at a given distance.
     * @param distance Distance from origin along ray direction.
     * @return Point at origin + distance * direction.
     */
    Vector3D GetPoint(float distance) const;

    /**
     * @brief Ensures the direction vector is normalized.
     * Call this if you manually modify the direction.
     */
    void Normalize();

    /**
     * @brief Checks if direction is normalized (unit length).
     * @param epsilon Tolerance for comparison (default 0.0001f).
     * @return True if direction magnitude is approximately 1.
     */
    bool IsNormalized(float epsilon = 0.0001f) const;

    /**
     * @brief Returns a transformed ray.
     * @param offset Translation to apply to origin.
     * @return New ray with translated origin.
     */
    Ray Translate(const Vector3D& offset) const;

    /**
     * @brief Creates a ray from two points.
     * @param from Start point (ray origin).
     * @param to End point (defines direction).
     * @return Ray from 'from' pointing towards 'to'.
     */
    static Ray FromPoints(const Vector3D& from, const Vector3D& to);

    /**
     * @brief Returns the closest point on this ray to a given point.
     * @param point The point to project onto the ray.
     * @return Closest point on ray (t >= 0).
     */
    Vector3D ClosestPoint(const Vector3D& point) const;

    /**
     * @brief Returns the distance parameter t for the closest point.
     * @param point The point to project onto the ray.
     * @return Distance t such that GetPoint(t) is closest to point.
     */
    float ClosestDistance(const Vector3D& point) const;

    KL_BEGIN_FIELDS(Ray)
        KL_FIELD(Ray, origin, "Origin", 0, 0),
        KL_FIELD(Ray, direction, "Direction", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Ray)
        KL_METHOD_AUTO(Ray, GetPoint, "Get point"),
        KL_METHOD_AUTO(Ray, Normalize, "Normalize"),
        KL_METHOD_AUTO(Ray, IsNormalized, "Is normalized"),
        KL_METHOD_AUTO(Ray, Translate, "Translate"),
        KL_METHOD_AUTO(Ray, ClosestPoint, "Closest point"),
        KL_METHOD_AUTO(Ray, ClosestDistance, "Closest distance"),
        KL_SMETHOD_AUTO(Ray::FromPoints, "From points")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Ray)
        KL_CTOR0(Ray),
        KL_CTOR(Ray, const Vector3D&, const Vector3D&),
        KL_CTOR(Ray, const Ray&)
    KL_END_DESCRIBE(Ray)
};

} // namespace koilo
