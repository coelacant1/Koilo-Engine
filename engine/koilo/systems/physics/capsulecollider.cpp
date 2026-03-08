// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/physics/capsulecollider.hpp>
#include <cmath>
#include <algorithm>

namespace koilo {

koilo::CapsuleCollider::CapsuleCollider()
    : Collider(ColliderType::Capsule), centerPosition(0, 0, 0),
      radius(0.5f), height(2.0f) {
}

koilo::CapsuleCollider::CapsuleCollider(const Vector3D& position, float radius, float height)
    : Collider(ColliderType::Capsule), centerPosition(position),
      radius(radius), height(height) {
}

koilo::CapsuleCollider::~CapsuleCollider() {
}

bool koilo::CapsuleCollider::Raycast(const Vector3D& origin, const Vector3D& direction,
                               RaycastHit& hit, float maxDistance) {
    return Raycast(Ray(origin, direction), hit, maxDistance);
}

void koilo::CapsuleCollider::SetRadius(float r) {
    if (r > 0) {
        radius = r;
    }
}

void koilo::CapsuleCollider::SetHeight(float h) {
    if (h > 0) {
        height = h;
    }
}

void koilo::CapsuleCollider::GetSegment(Vector3D& p1, Vector3D& p2) const {
    // Capsule is aligned along Y-axis
    float halfHeight = (height - 2.0f * radius) * 0.5f;
    halfHeight = std::max(0.0f, halfHeight);  // Ensure non-negative

    p1 = centerPosition + Vector3D(0, -halfHeight, 0);
    p2 = centerPosition + Vector3D(0, halfHeight, 0);
}

bool koilo::CapsuleCollider::Raycast(const Ray& ray,
                               RaycastHit& hit, float maxDistance) {
    // Simplified capsule raycast: treat as sphere for now
    // TODO: Proper capsule-ray intersection

    Vector3D oc = ray.origin - centerPosition;
    float a = ray.direction.DotProduct(ray.direction);
    float b = 2.0f * oc.DotProduct(ray.direction);
    float c = oc.DotProduct(oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return false;
    }

    float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    if (t < 0) {
        t = (-b + std::sqrt(discriminant)) / (2.0f * a);
    }

    if (t < 0 || t > maxDistance) {
        return false;
    }

    hit.distance = t;
    hit.point = ray.origin + ray.direction * t;
    hit.normal = (hit.point - centerPosition).Normal();
    hit.collider = this;

    return true;
}

bool koilo::CapsuleCollider::ContainsPoint(const Vector3D& point) {
    // Get capsule segment (line segment between hemisphere centers)
    Vector3D p1, p2;
    GetSegment(p1, p2);

    // Find closest point on segment to the test point
    Vector3D segment = p2 - p1;
    float segmentLength = segment.Magnitude();

    if (segmentLength < 1e-6f) {
        // Degenerate capsule (sphere)
        return (point - centerPosition).Magnitude() <= radius;
    }

    // Project point onto segment
    float t = (point - p1).DotProduct(segment) / (segmentLength * segmentLength);
    t = std::clamp(t, 0.0f, 1.0f);

    Vector3D closestOnSegment = p1 + segment * t;

    // Check distance from closest point on segment
    return (point - closestOnSegment).Magnitude() <= radius;
}

Vector3D koilo::CapsuleCollider::ClosestPoint(const Vector3D& point) {
    // Get capsule segment
    Vector3D p1, p2;
    GetSegment(p1, p2);

    // Find closest point on segment
    Vector3D segment = p2 - p1;
    float segmentLength = segment.Magnitude();

    if (segmentLength < 1e-6f) {
        // Degenerate capsule (sphere)
        Vector3D dir = point - centerPosition;
        float dist = dir.Magnitude();
        if (dist <= radius) {
            return point;
        }
        dir = dir.Normal();
        return centerPosition + dir * radius;
    }

    // Project point onto segment
    float t = (point - p1).DotProduct(segment) / (segmentLength * segmentLength);
    t = std::clamp(t, 0.0f, 1.0f);

    Vector3D closestOnSegment = p1 + segment * t;

    // Project to surface from segment
    Vector3D dir = point - closestOnSegment;
    float dist = dir.Magnitude();

    if (dist <= radius) {
        return point;  // Point is inside
    }

    dir = dir.Normal();
    return closestOnSegment + dir * radius;
}

Vector3D koilo::CapsuleCollider::GetPosition() const {
    return centerPosition;
}

void koilo::CapsuleCollider::SetPosition(const Vector3D& pos) {
    centerPosition = pos;
}

} // namespace koilo
