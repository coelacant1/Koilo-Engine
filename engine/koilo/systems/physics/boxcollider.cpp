// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/physics/boxcollider.hpp>
#include <algorithm>
#include <limits>

namespace koilo {

koilo::BoxCollider::BoxCollider()
    : Collider(ColliderType::Box), Cube(Vector3D(0, 0, 0), Vector3D(1, 1, 1)) {
    position = Vector3D(0, 0, 0);
}

koilo::BoxCollider::BoxCollider(const Vector3D& center, const Vector3D& size)
    : Collider(ColliderType::Box), Cube(center, size) {
    position = center;
}

koilo::BoxCollider::~BoxCollider() {
}

bool koilo::BoxCollider::Raycast(const Vector3D& origin, const Vector3D& direction,
                          RaycastHit& hit, float maxDistance) {
    return Raycast(Ray(origin, direction), hit, maxDistance);
}

bool koilo::BoxCollider::Raycast(const Ray& ray,
                          RaycastHit& hit, float maxDistance) {
    // AABB ray intersection algorithm (slab method)
    Vector3D min = GetMinimum();
    Vector3D max = GetMaximum();

    float tmin = -std::numeric_limits<float>::infinity();
    float tmax = std::numeric_limits<float>::infinity();

    // X-axis slab
    if (std::abs(ray.direction.X) > 1e-6f) {
        float tx1 = (min.X - ray.origin.X) / ray.direction.X;
        float tx2 = (max.X - ray.origin.X) / ray.direction.X;
        tmin = std::max(tmin, std::min(tx1, tx2));
        tmax = std::min(tmax, std::max(tx1, tx2));
    } else {
        // Ray parallel to slab, check if ray.origin is within slab
        if (ray.origin.X < min.X || ray.origin.X > max.X) {
            return false;
        }
    }

    // Y-axis slab
    if (std::abs(ray.direction.Y) > 1e-6f) {
        float ty1 = (min.Y - ray.origin.Y) / ray.direction.Y;
        float ty2 = (max.Y - ray.origin.Y) / ray.direction.Y;
        tmin = std::max(tmin, std::min(ty1, ty2));
        tmax = std::min(tmax, std::max(ty1, ty2));
    } else {
        if (ray.origin.Y < min.Y || ray.origin.Y > max.Y) {
            return false;
        }
    }

    // Z-axis slab
    if (std::abs(ray.direction.Z) > 1e-6f) {
        float tz1 = (min.Z - ray.origin.Z) / ray.direction.Z;
        float tz2 = (max.Z - ray.origin.Z) / ray.direction.Z;
        tmin = std::max(tmin, std::min(tz1, tz2));
        tmax = std::min(tmax, std::max(tz1, tz2));
    } else {
        if (ray.origin.Z < min.Z || ray.origin.Z > max.Z) {
            return false;
        }
    }

    // Check if ray intersects box
    if (tmax < tmin || tmin < 0 || tmin > maxDistance) {
        return false;
    }

    // Use tmin as hit distance (entry point)
    float t = tmin;

    // Fill hit information
    hit.distance = t;
    hit.point = ray.origin + ray.direction * t;
    hit.collider = this;

    // Calculate normal based on which face was hit
    Vector3D center = GetPosition();
    Vector3D localHit = hit.point - center;
    Vector3D halfSize = GetSize() * 0.5f;

    // Find which axis has the largest relative distance
    Vector3D absLocal(std::abs(localHit.X / halfSize.X),
                      std::abs(localHit.Y / halfSize.Y),
                      std::abs(localHit.Z / halfSize.Z));

    if (absLocal.X > absLocal.Y && absLocal.X > absLocal.Z) {
        hit.normal = Vector3D(localHit.X > 0 ? 1.0f : -1.0f, 0, 0);
    } else if (absLocal.Y > absLocal.Z) {
        hit.normal = Vector3D(0, localHit.Y > 0 ? 1.0f : -1.0f, 0);
    } else {
        hit.normal = Vector3D(0, 0, localHit.Z > 0 ? 1.0f : -1.0f);
    }

    return true;
}

bool koilo::BoxCollider::ContainsPoint(const Vector3D& point) {
    Vector3D min = GetMinimum();
    Vector3D max = GetMaximum();

    return (point.X >= min.X && point.X <= max.X &&
            point.Y >= min.Y && point.Y <= max.Y &&
            point.Z >= min.Z && point.Z <= max.Z);
}

Vector3D koilo::BoxCollider::ClosestPoint(const Vector3D& point) {
    Vector3D min = GetMinimum();
    Vector3D max = GetMaximum();

    return Vector3D(
        std::clamp(point.X, min.X, max.X),
        std::clamp(point.Y, min.Y, max.Y),
        std::clamp(point.Z, min.Z, max.Z)
    );
}

Vector3D koilo::BoxCollider::GetPosition() const {
    return position;
}

void koilo::BoxCollider::SetPosition(const Vector3D& pos) {
    position = pos;
}

} // namespace koilo
