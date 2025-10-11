#include <ptx/systems/physics/spherecollider.hpp>
#include <cmath>
#include <algorithm>

namespace ptx {

SphereCollider::SphereCollider()
    : Collider(ColliderType::Sphere), Sphere(Vector3D(0, 0, 0), 1.0f) {
}

SphereCollider::SphereCollider(const Vector3D& position, float radius)
    : Collider(ColliderType::Sphere), Sphere(position, radius) {
}

SphereCollider::~SphereCollider() {
}

bool SphereCollider::Raycast(const Vector3D& origin, const Vector3D& direction,
                              RaycastHit& hit, float maxDistance) {
    // Ray-sphere intersection using quadratic formula
    // Ray: P(t) = origin + t * direction
    // Sphere: |P - center|^2 = radius^2

    Vector3D oc = origin - position;
    float a = direction.dot(direction);
    float b = 2.0f * oc.dot(direction);
    float radius = GetRadius();
    float c = oc.dot(oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return false;  // No intersection
    }

    // Find nearest intersection point
    float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    if (t < 0) {
        // Ray starts inside sphere, use far intersection
        t = (-b + std::sqrt(discriminant)) / (2.0f * a);
    }

    if (t < 0 || t > maxDistance) {
        return false;  // Hit is behind ray or beyond max distance
    }

    // Fill hit information
    hit.distance = t;
    hit.point = origin + direction * t;
    hit.normal = (hit.point - position);
    hit.normal.normalize();
    hit.collider = this;

    return true;
}

bool SphereCollider::ContainsPoint(const Vector3D& point) {
    float radius = GetRadius();
    float distSquared = (point - position).lengthSquared();
    return distSquared <= (radius * radius);
}

Vector3D SphereCollider::ClosestPoint(const Vector3D& point) {
    Vector3D dir = point - position;
    float dist = dir.length();
    float radius = GetRadius();

    if (dist <= radius) {
        // Point is inside sphere
        return point;
    }

    // Point is outside, project to surface
    dir.normalize();
    return position + dir * radius;
}

Vector3D SphereCollider::GetPosition() const {
    return position;
}

void SphereCollider::SetPosition(const Vector3D& pos) {
    position = pos;
}

} // namespace ptx
