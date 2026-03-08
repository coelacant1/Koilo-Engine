// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/physics/spherecollider.hpp>
#include <cmath>

namespace koilo {

koilo::SphereCollider::SphereCollider()
    : Collider(ColliderType::Sphere), Sphere(Vector3D(0, 0, 0), 1.0f) {
}

koilo::SphereCollider::SphereCollider(const Vector3D& position, float radius)
    : Collider(ColliderType::Sphere), Sphere(position, radius) {
}

koilo::SphereCollider::~SphereCollider() {
}

bool koilo::SphereCollider::Raycast(const Ray& ray, RaycastHit& hit, float maxDistance) {
    // Ray-sphere intersection using quadratic formula
    // Ray: P(t) = origin + t * direction
    // Sphere: |P - center|^2 = radius^2

    Vector3D oc = ray.origin - position;
    float a = ray.direction.DotProduct(ray.direction);
    float b = 2.0f * oc.DotProduct(ray.direction);
    float radius = GetRadius();
    float c = oc.DotProduct(oc) - radius * radius;
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
    hit.point = ray.GetPoint(t);
    hit.normal = (hit.point - position).Normal();
    hit.collider = this;

    return true;
}

bool koilo::SphereCollider::ContainsPoint(const Vector3D& point) {
    float radius = GetRadius();
    Vector3D delta = point - position;
    float distSquared = delta.DotProduct(delta);
    return distSquared <= (radius * radius);
}

Vector3D koilo::SphereCollider::ClosestPoint(const Vector3D& point) {
    Vector3D dir = point - position;
    float dist = dir.Magnitude();
    float radius = GetRadius();

    if (dist <= radius) {
        // Point is inside sphere
        return point;
    }

    // Point is outside, project to surface
    dir = dir.Normal();
    return position + dir * radius;
}

Vector3D koilo::SphereCollider::GetPosition() const {
    return position;
}

void koilo::SphereCollider::SetPosition(const Vector3D& pos) {
    position = pos;
}

} // namespace koilo
