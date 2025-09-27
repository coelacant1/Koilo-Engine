#include <ptx/core/geometry/2d/circle.hpp>

Circle2D::Circle2D(Vector2D center, float radius) : Shape(center, Vector2D(radius * 2.0f, radius * 2.0f), 0.0f), radius(radius) {}

bool Circle2D::IsInShape(Vector2D point) {
    Vector2D center = GetCenter();
    
    float x = (point.X - center.X);
    float y = (point.Y - center.Y);
    float powRad = radius * radius;

    return x * x + y * y < powRad;
}
