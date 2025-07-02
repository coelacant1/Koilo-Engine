/**
 * @file Rectangle.h
 * @brief Defines the Rectangle class for representing rectangular shapes in 2D space.
 *
 * The Rectangle class provides functionality to define a rectangle by its center, size, and rotation,
 * and check if a given point lies within the rectangle's boundaries.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "shape.hpp"
#include "../../math/vector2d.hpp"
#include "../../math/mathematics.hpp"

/**
 * @class Rectangle
 * @brief Represents a rectangle in 2D space.
 */
class Rectangle2D : public Shape {
private:
    Vector2D minV; ///< Minimum corner of the bounding box.
    Vector2D maxV; ///< Maximum corner of the bounding box.
    Vector2D midV; ///< Center point of the bounding box.

public:
    struct Corners {
        Vector2D corners[4];
    };

    /**
     * @brief Constructs a Rectangle object with specified center, size, and rotation.
     * @param center Center point of the rectangle.
     * @param size Dimensions of the rectangle (width and height).
     * @param rotationDeg Rotation angle of the rectangle in degrees.
     */
    Rectangle2D(Vector2D center, Vector2D size, float rotationDeg = 0.0f);

    /**
     * @brief Constructs a Rectangle object with an input of bounds and rotation.
     * @param bounds Bounds for the new rectangle
     * @param rotationDeg Rotation angle of the rectangle in degrees.
     */
    Rectangle2D(Bounds bounds, float rotationDeg = 0.0f);

    /**
     * @brief Checks if a given point lies within the rectangle's boundaries.
     * @param point The point to check.
     * @return True if the point is within the rectangle, otherwise false.
     */
    bool IsInShape(Vector2D point) override;

    Corners GetCorners() const;

    void UpdateBounds(const Vector2D& v);

    Vector2D GetMinimum() const;
    Vector2D GetMaximum() const;
    Vector2D GetCenter() const;

    bool Overlaps(const Rectangle2D& other) const;
    bool Overlaps(const Vector2D& minI, const Vector2D& maxI) const;
    bool Contains(const Vector2D& v) const;
};
