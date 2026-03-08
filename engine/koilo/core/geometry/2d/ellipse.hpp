// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file Ellipse.h
 * @brief Defines the Ellipse class for representing elliptical shapes in 2D space.
 *
 * The Ellipse class provides functionality to define an ellipse by its center, size, and rotation,
 * and check if a given point lies within the ellipse's boundaries.
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
 * @class Ellipse
 * @brief Represents an ellipse in 2D space.
 */
class Ellipse2D : public Shape {
public:
    /**
     * @brief Constructs an Ellipse object with specified center, size, and rotation.
     * @param center Center point of the ellipse.
     * @param size Dimensions of the ellipse (width and height).
     * @param rotation Rotation angle of the ellipse in degrees.
     */
    Ellipse2D(Vector2D center, Vector2D size, float rotation = 0.0f);

    /**
     * @brief Constructs an Ellipse object with specified center, size, and rotation.
     * @param bounds Ellipse bounds.
     * @param rotation Rotation angle of the ellipse in degrees.
     */
    Ellipse2D(Bounds bounds, float rotation = 0.0f);

    /**
     * @brief Checks if a given point lies within the ellipse's boundaries.
     * @param point The point to check.
     * @return True if the point is within the ellipse, otherwise false.
     */
    bool IsInShape(Vector2D point) override;

    KL_BEGIN_FIELDS(Ellipse2D)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Ellipse2D)
        KL_METHOD_AUTO(Ellipse2D, IsInShape, "Is in shape")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Ellipse2D)
        KL_CTOR(Ellipse2D, Vector2D, Vector2D, float),
        KL_CTOR(Ellipse2D, Bounds, float)
    KL_END_DESCRIBE(Ellipse2D)

};

} // namespace koilo
