/**
 * @file shape.cpp
 * @brief Implements the Shape base-class declared in shape.hpp.
 * @date  18/06/2025
 * @author Coela Can't
 */

#include <ptx/core/geometry/2d/shape.hpp>

Shape::Shape(Vector2D center, Vector2D size, float rotationDeg) : rotation(rotationDeg) {
    Vector2D half = { size.X * 0.5f, size.Y * 0.5f };

    bounds.minV   = { center.X - half.X, center.Y - half.Y };
    bounds.maxV   = { center.X + half.X, center.Y + half.Y };
}

Shape::Shape(Bounds b, float rotationDeg) : bounds(b), rotation(rotationDeg) {}


void Shape::SetCenter(Vector2D center) {
    Vector2D half = { (bounds.maxV.X - bounds.minV.X) * 0.5f,
                      (bounds.maxV.Y - bounds.minV.Y) * 0.5f };

    bounds.minV = { center.X - half.X, center.Y - half.Y };
    bounds.maxV = { center.X + half.X, center.Y + half.Y };
}

void Shape::SetBounds(Bounds b) {
    bounds = b;
}

void Shape::Translate(Vector2D offset) {
    bounds.minV = bounds.minV + offset;
    bounds.maxV = bounds.maxV + offset;
}

Vector2D Shape::GetCenter() const {
    return { (bounds.minV.X + bounds.maxV.X) * 0.5f,
             (bounds.minV.Y + bounds.maxV.Y) * 0.5f };
}

Shape::Bounds Shape::GetBounds() const { 
    return bounds;
}

void Shape::SetSize(Vector2D size) {
    Vector2D cen  = GetCenter();
    Vector2D half = { size.X * 0.5f, size.Y * 0.5f };
    
    bounds.minV   = { cen.X - half.X, cen.Y - half.Y };
    bounds.maxV   = { cen.X + half.X, cen.Y + half.Y };
}

void Shape::Scale(Vector2D factor) {
    Vector2D cen  = GetCenter();
    Vector2D half = { (bounds.maxV.X - bounds.minV.X) * 0.5f * factor.X,
                      (bounds.maxV.Y - bounds.minV.Y) * 0.5f * factor.Y };

    bounds.minV   = { cen.X - half.X, cen.Y - half.Y };
    bounds.maxV   = { cen.X + half.X, cen.Y + half.Y };
}

Vector2D Shape::GetSize() const {
    return { bounds.maxV.X - bounds.minV.X,
             bounds.maxV.Y - bounds.minV.Y };
}

void Shape::SetRotation(float rotationDeg) {
    rotation = rotationDeg;
}

void Shape::Rotate(float offsetDeg) {
    rotation += offsetDeg;
}

float Shape::GetRotation() const {
    return rotation;
}

bool Shape::Overlaps(Shape& shape) const{
    return !(bounds.maxV.X < shape.bounds.minV.X ||
             bounds.minV.X > shape.bounds.maxV.X ||
             bounds.maxV.Y < shape.bounds.minV.Y ||
             bounds.minV.Y > shape.bounds.maxV.Y);
}
