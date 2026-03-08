// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file Circle.h
 * @brief Defines the Circle class for representing circular shapes in 2D space.
 *
 * The Circle class provides functionality to define a circle by its center and radius,
 * and check if a given point lies within the circle's boundaries.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "shape.hpp"
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @class Circle
 * @brief Represents a circle in 2D space.
 */
class Circle2D : public Shape {
private:
    float radius; ///< Radius of the circle.

public:
    /**
     * @brief Constructs a Circle object with a specified center and radius.
     * @param center Center point of the circle.
     * @param radius Radius of the circle.
     */
    Circle2D(Vector2D center, float radius);

    /**
     * @brief Checks if a given point lies within the circle's boundaries.
     * @param point The point to check.
     * @return True if the point is within the circle, otherwise false.
     */
    bool IsInShape(Vector2D point) override;

    /** @brief Returns radius of the circle. */
    float GetRadius() const { return radius; }

    /** @brief Circle-circle overlap via distance check. */
    bool OverlapsCircle(const Circle2D& other) const {
        Vector2D ca = GetCenter();
        Vector2D cb = other.GetCenter();
        float dx = ca.X - cb.X;
        float dy = ca.Y - cb.Y;
        float r = radius + other.radius;
        return (dx * dx + dy * dy) <= r * r;
    }

    KL_BEGIN_FIELDS(Circle2D)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Circle2D)
        KL_METHOD_AUTO(Circle2D, IsInShape, "Is in shape"),
        KL_METHOD_AUTO(Circle2D, OverlapsCircle, "Overlaps circle")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Circle2D)
        KL_CTOR(Circle2D, Vector2D, float)
    KL_END_DESCRIBE(Circle2D)

};

} // namespace koilo
