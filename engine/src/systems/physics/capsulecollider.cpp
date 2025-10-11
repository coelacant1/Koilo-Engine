#include <ptx/systems/physics/capsulecollider.hpp>
#include <cmath>
#include <algorithm>

namespace ptx {

CapsuleCollider::CapsuleCollider()
    : Collider(ColliderType::Capsule), centerPosition(0, 0, 0),
      radius(0.5f), height(2.0f) {
}

CapsuleCollider::CapsuleCollider(const Vector3D& position, float radius, float height)
    : Collider(ColliderType::Capsule), centerPosition(position),
      radius(radius), height(height) {
}

CapsuleCollider::~CapsuleCollider() {
}

void CapsuleCollider::SetRadius(float r) {
    if (r > 0) {
        radius = r;
    }
}

void CapsuleCollider::SetHeight(float h) {
    if (h > 0) {
        height = h;
    }
}

void CapsuleCollider::GetSegment(Vector3D& p1, Vector3D& p2) const {
    // Capsule is aligned along Y-axis
    float halfHeight = (height - 2.0f * radius) * 0.5f;
    halfHeight = std::max(0.0f, halfHeight);  // Ensure non-negative

    p1 = centerPosition + Vector3D(0, -halfHeight, 0);
    p2 = centerPosition + Vector3D(0, halfHeight, 0);
}

bool CapsuleCollider::Raycast(const Vector3D& origin, const Vector3D& direction,
                               RaycastHit& hit, float maxDistance) {
    // Simplified capsule raycast: treat as sphere for now
    // TODO: Implement proper capsule-ray intersection

    Vector3D oc = origin - centerPosition;
    float a = direction.dot(direction);
    float b = 2.0f * oc.dot(direction);
    float c = oc.dot(oc) - radius * radius;
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
    hit.point = origin + direction * t;
    hit.normal = (hit.point - centerPosition);
    hit.normal.normalize();
    hit.collider = this;

    return true;
}

bool CapsuleCollider::ContainsPoint(const Vector3D& point) {
    // Get capsule segment (line segment between hemisphere centers)
    Vector3D p1, p2;
    GetSegment(p1, p2);

    // Find closest point on segment to the test point
    Vector3D segment = p2 - p1;
    float segmentLength = segment.length();

    if (segmentLength < 1e-6f) {
        // Degenerate capsule (sphere)
        return (point - centerPosition).length() <= radius;
    }

    // Project point onto segment
    float t = (point - p1).dot(segment) / (segmentLength * segmentLength);
    t = std::clamp(t, 0.0f, 1.0f);

    Vector3D closestOnSegment = p1 + segment * t;

    // Check distance from closest point on segment
    return (point - closestOnSegment).length() <= radius;
}

Vector3D CapsuleCollider::ClosestPoint(const Vector3D& point) {
    // Get capsule segment
    Vector3D p1, p2;
    GetSegment(p1, p2);

    // Find closest point on segment
    Vector3D segment = p2 - p1;
    float segmentLength = segment.length();

    if (segmentLength < 1e-6f) {
        // Degenerate capsule (sphere)
        Vector3D dir = point - centerPosition;
        float dist = dir.length();
        if (dist <= radius) {
            return point;
        }
        dir.normalize();
        return centerPosition + dir * radius;
    }

    // Project point onto segment
    float t = (point - p1).dot(segment) / (segmentLength * segmentLength);
    t = std::clamp(t, 0.0f, 1.0f);

    Vector3D closestOnSegment = p1 + segment * t;

    // Project to surface from segment
    Vector3D dir = point - closestOnSegment;
    float dist = dir.length();

    if (dist <= radius) {
        return point;  // Point is inside
    }

    dir.normalize();
    return closestOnSegment + dir * radius;
}

Vector3D CapsuleCollider::GetPosition() const {
    return centerPosition;
}

void CapsuleCollider::SetPosition(const Vector3D& pos) {
    centerPosition = pos;
}

} // namespace ptx
