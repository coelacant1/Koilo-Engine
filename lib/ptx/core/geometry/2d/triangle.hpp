/**
 * @file triangle2d.hpp
 * @brief 2-D analytic triangle
 * @date  18/06/2025
 * @author Coela Can't
 */
#pragma once

#include "shape.hpp"
#include "../../math/vector2d.hpp"
#include "../../../registry/reflect_macros.hpp"

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

    PTX_BEGIN_FIELDS(Triangle2D)
        PTX_FIELD(Triangle2D, p1, "P1", 0, 0),
        PTX_FIELD(Triangle2D, p2, "P2", 0, 0),
        PTX_FIELD(Triangle2D, p3, "P3", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Triangle2D)
        PTX_METHOD_AUTO(Triangle2D, GetArea, "Get area"),
        PTX_METHOD_AUTO(Triangle2D, GetCentroid, "Get centroid"),
        PTX_METHOD_AUTO(Triangle2D, IsInShape, "Is in shape")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Triangle2D)
        PTX_CTOR0(Triangle2D),
        PTX_CTOR(Triangle2D, const Vector2D &, const Vector2D &, const Vector2D &)
    PTX_END_DESCRIBE(Triangle2D)

};
