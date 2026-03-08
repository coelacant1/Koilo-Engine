// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file triangle3d.hpp
 * @brief A self-contained, value-based 3D triangle for pure geometric queries.
 * @date  26/06/2025
 * @author Coela Can't
 */
#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @class Triangle3D
 * @brief Represents a triangle in 3D space by storing pointers to its vertices.
 *
 * This class is designed for dynamic meshes where vertices may be modified.
 * It stores pointers to vertices rather than copies, allowing transforms to work correctly.
 */
class Triangle3D {
public:
    Vector3D* p1; ///< Pointer to first vertex.
    Vector3D* p2; ///< Pointer to second vertex.
    Vector3D* p3; ///< Pointer to third vertex.

    /** @brief Default constructor creates a triangle with null pointers. */
    Triangle3D();

    /** @brief Constructs a triangle from three vertex pointers. */
    Triangle3D(Vector3D* v1, Vector3D* v2, Vector3D* v3);

    /** @brief Calculates the surface area of the triangle. */
    float GetArea() const;

    /** @brief Calculates the normalized surface normal of the triangle. */
    Vector3D GetNormal() const;

    /** @brief Calculates the geometric center (centroid) of the triangle. */
    Vector3D GetCentroid() const;

    /**
     * @brief Finds the closest point on the surface of the triangle to a given point p.
     * @param p The point to test against.
     * @return The point on the triangle's surface closest to p.
     */
    Vector3D ClosestPoint(const Vector3D& p) const;

    KL_BEGIN_FIELDS(Triangle3D)
        KL_FIELD(Triangle3D, p1, "P1", 0, 0),
        KL_FIELD(Triangle3D, p2, "P2", 0, 0),
        KL_FIELD(Triangle3D, p3, "P3", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Triangle3D)
        KL_METHOD_AUTO(Triangle3D, GetArea, "Get area"),
        KL_METHOD_AUTO(Triangle3D, GetNormal, "Get normal"),
        KL_METHOD_AUTO(Triangle3D, GetCentroid, "Get centroid"),
        KL_METHOD_AUTO(Triangle3D, ClosestPoint, "Closest point")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Triangle3D)
        KL_CTOR0(Triangle3D),
        KL_CTOR(Triangle3D, Vector3D *, Vector3D *, Vector3D *)
    KL_END_DESCRIBE(Triangle3D)

};

} // namespace koilo