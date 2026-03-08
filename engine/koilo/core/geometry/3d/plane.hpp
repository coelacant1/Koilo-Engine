// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file Plane.h
 * @brief Defines the Plane class for representing a plane in 3D space.
 *
 * The Plane class represents a mathematical plane defined by a centroid and a normal vector.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <koilo/core/math/mathematics.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/geometry/ray.hpp>
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @class Plane
 * @brief Represents a plane in 3D space defined by a centroid and a normal vector.
 *
 * The `Plane` class provides methods for initializing and describing a plane
 * in three-dimensional space using a point on the plane (centroid) and a normal vector.
 */
class Plane {
public:
    Vector3D Centroid; ///< Point on the plane, representing the centroid.
    Vector3D Normal;   ///< Normal vector defining the plane's orientation.

    /**
     * @brief Default constructor.
     *
     * Initializes the plane with a default centroid and normal vector.
     */
    Plane();

    /**
     * @brief Parameterized constructor.
     *
     * Initializes the plane with the specified centroid and normal vector.
     *
     * @param centroid A point on the plane (centroid).
     * @param normal A vector normal to the plane.
     */
    Plane(Vector3D centroid, Vector3D normal);

    /**
     * @brief Converts the Plane object to a string representation.
     *
     * @return A string describing the plane's centroid and normal vector.
     */
    koilo::UString ToString();

    /**
     * @brief Returns signed distance from a point to this plane.
     * Positive = same side as normal, negative = opposite side.
     */
    float DistanceToPoint(const Vector3D& point) const;

    /**
     * @brief Checks if a ray intersects this plane.
     * @param ray The ray to test.
     * @param outT Output: distance along ray to intersection (only valid if returns true).
     * @return True if the ray intersects (not parallel).
     */
    bool RayIntersect(const Ray& ray, float& outT) const;

    /**
     * @brief Returns the closest point on the plane to a given point.
     */
    Vector3D ClosestPoint(const Vector3D& point) const;

    /**
     * @brief Returns the side of the plane a point is on.
     * @return +1 if on normal side, -1 if opposite, 0 if on the plane.
     */
    int Side(const Vector3D& point) const;

    KL_BEGIN_FIELDS(Plane)
        KL_FIELD(Plane, Centroid, "Centroid", 0, 0),
        KL_FIELD(Plane, Normal, "Normal", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Plane)
        KL_METHOD_AUTO(Plane, ToString, "To string"),
        KL_METHOD_AUTO(Plane, DistanceToPoint, "Distance to point"),
        KL_METHOD_AUTO(Plane, ClosestPoint, "Closest point"),
        KL_METHOD_AUTO(Plane, Side, "Side")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Plane)
        KL_CTOR0(Plane),
        KL_CTOR(Plane, Vector3D, Vector3D)
    KL_END_DESCRIBE(Plane)

};

} // namespace koilo
