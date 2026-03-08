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
#include "../../../registry/reflect_macros.hpp"

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

    PTX_BEGIN_FIELDS(Circle2D)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(Circle2D)
        PTX_METHOD_AUTO(Circle2D, IsInShape, "Is in shape")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(Circle2D)
        PTX_CTOR(Circle2D, Vector2D, float)
    PTX_END_DESCRIBE(Circle2D)

};
