#include <ptx/systems/physics/boxcollider.hpp>
#include <algorithm>
#include <limits>

namespace ptx {

BoxCollider::BoxCollider()
    : Collider(ColliderType::Box), Cube(Vector3D(0, 0, 0), Vector3D(1, 1, 1)) {
}

BoxCollider::BoxCollider(const Vector3D& center, const Vector3D& size)
    : Collider(ColliderType::Box), Cube(center, size) {
}

BoxCollider::~BoxCollider() {
}

bool BoxCollider::Raycast(const Vector3D& origin, const Vector3D& direction,
                          RaycastHit& hit, float maxDistance) {
    // AABB ray intersection algorithm (slab method)
    Vector3D min = GetMinimum();
    Vector3D max = GetMaximum();

    float tmin = -std::numeric_limits<float>::infinity();
    float tmax = std::numeric_limits<float>::infinity();

    // X-axis slab
    if (std::abs(direction.x) > 1e-6f) {
        float tx1 = (min.x - origin.x) / direction.x;
        float tx2 = (max.x - origin.x) / direction.x;
        tmin = std::max(tmin, std::min(tx1, tx2));
        tmax = std::min(tmax, std::max(tx1, tx2));
    } else {
        // Ray parallel to slab, check if origin is within slab
        if (origin.x < min.x || origin.x > max.x) {
            return false;
        }
    }

    // Y-axis slab
    if (std::abs(direction.y) > 1e-6f) {
        float ty1 = (min.y - origin.y) / direction.y;
        float ty2 = (max.y - origin.y) / direction.y;
        tmin = std::max(tmin, std::min(ty1, ty2));
        tmax = std::min(tmax, std::max(ty1, ty2));
    } else {
        if (origin.y < min.y || origin.y > max.y) {
            return false;
        }
    }

    // Z-axis slab
    if (std::abs(direction.z) > 1e-6f) {
        float tz1 = (min.z - origin.z) / direction.z;
        float tz2 = (max.z - origin.z) / direction.z;
        tmin = std::max(tmin, std::min(tz1, tz2));
        tmax = std::min(tmax, std::max(tz1, tz2));
    } else {
        if (origin.z < min.z || origin.z > max.z) {
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
    hit.point = origin + direction * t;
    hit.collider = this;

    // Calculate normal based on which face was hit
    Vector3D center = GetPosition();
    Vector3D localHit = hit.point - center;
    Vector3D halfSize = GetSize() * 0.5f;

    // Find which axis has the largest relative distance
    Vector3D absLocal(std::abs(localHit.x / halfSize.x),
                      std::abs(localHit.y / halfSize.y),
                      std::abs(localHit.z / halfSize.z));

    if (absLocal.x > absLocal.y && absLocal.x > absLocal.z) {
        hit.normal = Vector3D(localHit.x > 0 ? 1.0f : -1.0f, 0, 0);
    } else if (absLocal.y > absLocal.z) {
        hit.normal = Vector3D(0, localHit.y > 0 ? 1.0f : -1.0f, 0);
    } else {
        hit.normal = Vector3D(0, 0, localHit.z > 0 ? 1.0f : -1.0f);
    }

    return true;
}

bool BoxCollider::ContainsPoint(const Vector3D& point) {
    Vector3D min = GetMinimum();
    Vector3D max = GetMaximum();

    return (point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z);
}

Vector3D BoxCollider::ClosestPoint(const Vector3D& point) {
    Vector3D min = GetMinimum();
    Vector3D max = GetMaximum();

    return Vector3D(
        std::clamp(point.x, min.x, max.x),
        std::clamp(point.y, min.y, max.y),
        std::clamp(point.z, min.z, max.z)
    );
}

Vector3D BoxCollider::GetPosition() const {
    return position;
}

void BoxCollider::SetPosition(const Vector3D& pos) {
    position = pos;
}

} // namespace ptx
