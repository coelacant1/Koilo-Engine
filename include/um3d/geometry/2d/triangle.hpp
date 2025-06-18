/**
 * @file triangle2d.hpp
 * @brief 2-D analytic triangle
 * @date  18/06/2025
 * @author Coela Can't
 */
#pragma once

#include "shape.hpp"
#include "../../math/vector2d.hpp"

/**
 * @class Triangle2D
 * @brief Immutable three-point triangle for geometry queries.
 */
class Triangle2D : public Shape{
public:
    Vector2D p1;  ///< first vertex
    Vector2D p2;  ///< second vertex
    Vector2D p3;  ///< third vertex

    /** @brief Default ctor (degenerate triangle at origin). */
    Triangle2D();

    /** @brief Construct from explicit vertices. */
    Triangle2D(const Vector2D& p1In, const Vector2D& p2In, const Vector2D& p3In);

    /** @brief Area of the triangle. */
    float GetArea() const;

    /** @brief Centroid (barycentric average). */
    Vector2D GetCentroid() const;

    /**
     * @brief Point-inside test via barycentric coordinates.
     * @param x input x
     * @param y input y
     * @return true if (x,y) lies in or on the triangle
     */
    bool Contains(float x, float y) const;
};
