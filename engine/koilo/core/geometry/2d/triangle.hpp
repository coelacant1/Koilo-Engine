// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file triangle2d.hpp
 * @brief 2-D analytic triangle
 * @date  18/06/2025
 * @author Coela Can't
 */
#pragma once

#include "shape.hpp"
#include <koilo/core/math/vector2d.hpp>
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @class Triangle2D
 * @brief Immutable three-point triangle for geometry queries.
 */
class Triangle2D : public Shape{
public:
    Vector2D p1;  ///< first vertex
    Vector2D p2;  ///< second vertex
    Vector2D p3;  ///< third vertex

    /** @brief Default triangle at origin. */
    Triangle2D();

    /** @brief Construct from explicit vertices. */
    Triangle2D(const Vector2D& p1In, const Vector2D& p2In, const Vector2D& p3In);

    /** @brief Area of the triangle. */
    float GetArea() const;

    /** @brief Centroid (barycentric average). */
    Vector2D GetCentroid() const;

    /**
     * @brief Point-inside test via barycentric coordinates.
     * @param Vector2D point for X/Y input
     * @return true if (x,y) lies in or on the triangle
     */
    bool IsInShape(Vector2D point) override;

    KL_BEGIN_FIELDS(Triangle2D)
        KL_FIELD(Triangle2D, p1, "P1", 0, 0),
        KL_FIELD(Triangle2D, p2, "P2", 0, 0),
        KL_FIELD(Triangle2D, p3, "P3", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Triangle2D)
        KL_METHOD_AUTO(Triangle2D, GetArea, "Get area"),
        KL_METHOD_AUTO(Triangle2D, GetCentroid, "Get centroid"),
        KL_METHOD_AUTO(Triangle2D, IsInShape, "Is in shape")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Triangle2D)
        KL_CTOR0(Triangle2D),
        KL_CTOR(Triangle2D, const Vector2D &, const Vector2D &, const Vector2D &)
    KL_END_DESCRIBE(Triangle2D)

};

} // namespace koilo
