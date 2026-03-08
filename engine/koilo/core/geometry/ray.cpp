// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/geometry/ray.hpp>
#include <cmath>

namespace koilo {

koilo::Ray::Ray()
    : origin(0, 0, 0), direction(0, 0, 1) {
}

koilo::Ray::Ray(const Vector3D& origin, const Vector3D& direction)
    : origin(origin), direction(direction) {
    Normalize();
}

Vector3D koilo::Ray::GetPoint(float distance) const {
    return origin + direction * distance;
}

void koilo::Ray::Normalize() {
    float mag = direction.Magnitude();
    if (mag > 0.0f) {
        direction = direction / mag;
    } else {
        // Default to +Z if direction is zero
        direction = Vector3D(0, 0, 1);
    }
}

bool koilo::Ray::IsNormalized(float epsilon) const {
    float mag = direction.Magnitude();
    return std::abs(mag - 1.0f) < epsilon;
}

Ray koilo::Ray::Translate(const Vector3D& offset) const {
    return Ray(origin + offset, direction);
}

Ray koilo::Ray::FromPoints(const Vector3D& from, const Vector3D& to) {
    Vector3D dir = to - from;
    return Ray(from, dir);
}

Vector3D koilo::Ray::ClosestPoint(const Vector3D& point) const {
    float t = ClosestDistance(point);
    // Clamp t to non-negative values (ray starts at origin)
    if (t < 0.0f) {
        t = 0.0f;
    }
    return GetPoint(t);
}

float koilo::Ray::ClosestDistance(const Vector3D& point) const {
    Vector3D toPoint = point - origin;
    // Project onto direction
    float t = toPoint.DotProduct(direction);
    return t;
}

} // namespace koilo
