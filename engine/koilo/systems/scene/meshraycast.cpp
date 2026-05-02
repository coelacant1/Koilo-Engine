// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/meshraycast.hpp>
#include <koilo/systems/scene/mesh_bvh.hpp>
#include <cmath>

namespace koilo {

bool koilo::MeshRaycast::Raycast(const Ray& ray,
                          Mesh* mesh,
                          RaycastHit& hit,
                          float maxDistance,
                          bool backfaceCulling) {
    if (!mesh || !mesh->IsEnabled()) {
        return false;
    }

    ITriangleGroup* triangles = mesh->GetTriangleGroup();
    if (!triangles) {
        return false;
    }

    // Fast path: use the mesh's lazily-built BVH (O(log N)) when available.
    MeshBVH* accel = mesh->GetOrBuildRaycastAccel();
    if (accel && !accel->Empty()) {
        float dist;
        Vector3D point, normal;
        if (accel->Intersect(ray, maxDistance, backfaceCulling,
                              dist, point, normal)) {
            hit.distance = dist;
            hit.point    = point;
            hit.normal   = normal;
            hit.collider = nullptr;
            return true;
        }
        return false;
    }

    // Fallback: linear scan (used if BVH build returned empty / unavailable).
    const Vector3D* vertices = triangles->GetVertices();
    const IndexGroup* indices = triangles->GetIndexGroup();
    int triangleCount = triangles->GetTriangleCount();

    bool hasHit = false;
    float closestDistance = maxDistance;
    Vector3D closestPoint;
    Vector3D closestNormal;

    // Test each triangle
    for (int i = 0; i < triangleCount; ++i) {
        const IndexGroup& tri = indices[i];

        Vector3D v0 = vertices[tri.GetIndex(0)];
        Vector3D v1 = vertices[tri.GetIndex(1)];
        Vector3D v2 = vertices[tri.GetIndex(2)];

        float distance;
        Vector3D hitPoint;
        Vector3D normal;

        if (RaycastTriangle(ray, v0, v1, v2, distance, hitPoint, normal, backfaceCulling)) {
            if (distance < closestDistance) {
                closestDistance = distance;
                closestPoint = hitPoint;
                closestNormal = normal;
                hasHit = true;
            }
        }
    }

    if (hasHit) {
        hit.distance = closestDistance;
        hit.point = closestPoint;
        hit.normal = closestNormal;
        hit.collider = nullptr;  // Mesh hits don't have colliders
    }

    return hasHit;
}

bool koilo::MeshRaycast::RaycastTriangle(const Ray& ray,
                                  const Vector3D& v0,
                                  const Vector3D& v1,
                                  const Vector3D& v2,
                                  float& distance,
                                  Vector3D& hitPoint,
                                  Vector3D& normal,
                                  bool backfaceCulling) {
    // Möller-Trumbore ray-triangle intersection algorithm
    // Based on: "Fast, Minimum Storage Ray/Triangle Intersection" (1997)

    Vector3D edge1 = v1 - v0;
    Vector3D edge2 = v2 - v0;

    // Begin calculating determinant - also used to calculate u parameter
    Vector3D h = ray.direction.CrossProduct(edge2);
    float a = edge1.DotProduct(h);

    // If determinant is near zero, ray lies in plane of triangle or is parallel
    if (backfaceCulling) {
        if (a < EPSILON) {
            return false;  // Backface or parallel
        }
    } else {
        if (std::abs(a) < EPSILON) {
            return false;  // Parallel
        }
    }

    float f = 1.0f / a;
    Vector3D s = ray.origin - v0;
    float u = f * s.DotProduct(h);

    // Check if intersection is outside triangle (barycentric coordinate u)
    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    Vector3D q = s.CrossProduct(edge1);
    float v = f * ray.direction.DotProduct(q);

    // Check if intersection is outside triangle (barycentric coordinate v)
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    // At this stage we can compute t to find out where the intersection point is on the line
    float t = f * edge2.DotProduct(q);

    if (t > EPSILON) {
        // Ray intersection
        distance = t;
        hitPoint = ray.GetPoint(t);

        // Calculate normal
        normal = edge1.CrossProduct(edge2).Normal();

        return true;
    }

    // Line intersection but not ray intersection (t < 0)
    return false;
}

void koilo::MeshRaycast::ComputeBarycentric(const Vector3D& p,
                                    const Vector3D& a,
                                    const Vector3D& b,
                                    const Vector3D& c,
                                    float& u, float& v, float& w) {
    Vector3D v0 = b - a;
    Vector3D v1 = c - a;
    Vector3D v2 = p - a;

    float d00 = v0.DotProduct(v0);
    float d01 = v0.DotProduct(v1);
    float d11 = v1.DotProduct(v1);
    float d20 = v2.DotProduct(v0);
    float d21 = v2.DotProduct(v1);

    float denom = d00 * d11 - d01 * d01;

    if (std::abs(denom) < EPSILON) {
        // Degenerate triangle
        u = v = w = 1.0f / 3.0f;
        return;
    }

    v = (d11 * d20 - d01 * d21) / denom;
    w = (d00 * d21 - d01 * d20) / denom;
    u = 1.0f - v - w;
}

} // namespace koilo
