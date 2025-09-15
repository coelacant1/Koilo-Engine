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
#include "../../../registry/reflect_macros.hpp"

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
    
        PTX_BEGIN_FIELDS(Corners)
            PTX_FIELD(Corners, corners, "Corners", 0, 0)
        PTX_END_FIELDS
        
        PTX_BEGIN_METHODS(Corners)
            /* TODO: PTX_METHOD_AUTO(Corners, Method, "Doc") */
        PTX_END_METHODS
        
        PTX_BEGIN_DESCRIBE(Corners)
            /* TODO: PTX_CTOR0(Corners) or PTX_CTOR(Corners, ...) */
        PTX_END_DESCRIBE(Corners)
        
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

    PTX_BEGIN_FIELDS(Rectangle2D)
        /* TODO: PTX_FIELD(Rectangle2D, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(Rectangle2D)
        PTX_METHOD_AUTO(Rectangle2D, IsInShape, "Is in shape"),
        PTX_METHOD_AUTO(Rectangle2D, GetCorners, "Get corners"),
        PTX_METHOD_AUTO(Rectangle2D, UpdateBounds, "Update bounds"),
        PTX_METHOD_AUTO(Rectangle2D, GetMinimum, "Get minimum"),
        PTX_METHOD_AUTO(Rectangle2D, GetMaximum, "Get maximum"),
        PTX_METHOD_AUTO(Rectangle2D, GetCenter, "Get center"),
        PTX_METHOD_AUTO(Rectangle2D, Overlaps, "Overlaps"),
        PTX_METHOD_AUTO(Rectangle2D, Overlaps, "Overlaps"),
        PTX_METHOD_AUTO(Rectangle2D, Contains, "Contains")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(Rectangle2D)
        PTX_CTOR(Rectangle2D, Vector2D, Vector2D, float),
        PTX_CTOR(Rectangle2D, Bounds, float)
    PTX_END_DESCRIBE(Rectangle2D)
    
};
