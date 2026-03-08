// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/geometry/3d/plane.hpp>
#include <cmath>


namespace koilo {

koilo::Plane::Plane() {}

koilo::Plane::Plane(Vector3D centroid, Vector3D normal)
    : Centroid(centroid), Normal(normal) {}

koilo::UString koilo::Plane::ToString() {
    koilo::UString centroid = Centroid.ToString();
    koilo::UString normal = Normal.ToString();
    return "[ " + centroid + ", " + normal + " ]";
}

float koilo::Plane::DistanceToPoint(const Vector3D& point) const {
    Vector3D diff = point - Centroid;
    return diff.X * Normal.X + diff.Y * Normal.Y + diff.Z * Normal.Z;
}

bool koilo::Plane::RayIntersect(const Ray& ray, float& outT) const {
    float denom = ray.direction.X * Normal.X +
                  ray.direction.Y * Normal.Y +
                  ray.direction.Z * Normal.Z;
    if (std::abs(denom) < 1e-7f) return false; // parallel
    Vector3D diff = Centroid - ray.origin;
    outT = (diff.X * Normal.X + diff.Y * Normal.Y + diff.Z * Normal.Z) / denom;
    return outT >= 0.0f;
}

Vector3D koilo::Plane::ClosestPoint(const Vector3D& point) const {
    float d = DistanceToPoint(point);
    return point - Normal * d;
}

int koilo::Plane::Side(const Vector3D& point) const {
    float d = DistanceToPoint(point);
    if (d > 1e-6f) return 1;
    if (d < -1e-6f) return -1;
    return 0;
}

} // namespace koilo
