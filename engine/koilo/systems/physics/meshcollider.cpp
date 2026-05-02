// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/physics/meshcollider.hpp>
#include <koilo/systems/physics/raycasthit.hpp>
#include <koilo/core/geometry/ray.hpp>

#include <cmath>
#include <limits>

namespace koilo {

MeshCollider::MeshCollider() : Collider(ColliderType::Mesh) {}

MeshCollider::MeshCollider(std::shared_ptr<const TriangleMeshData> data)
    : Collider(ColliderType::Mesh), data_(std::move(data)) {}

MeshCollider::~MeshCollider() = default;

namespace {

inline float Dot(const Vector3D& a, const Vector3D& b) {
    return a.X*b.X + a.Y*b.Y + a.Z*b.Z;
}
inline Vector3D Cross(const Vector3D& a, const Vector3D& b) {
    return Vector3D(a.Y*b.Z - a.Z*b.Y,
                    a.Z*b.X - a.X*b.Z,
                    a.X*b.Y - a.Y*b.X);
}

// Möller-Trumbore ray-triangle intersection. Double-sided. Writes hit-t and
// triangle-face normal (winding-based) on hit.
inline bool RayTriangle(const Vector3D& orig, const Vector3D& dir,
                        const Vector3D& v0, const Vector3D& v1, const Vector3D& v2,
                        float& tOut, Vector3D& nOut) {
    const Vector3D e1 = v1 - v0;
    const Vector3D e2 = v2 - v0;
    const Vector3D p  = Cross(dir, e2);
    const float    det = Dot(e1, p);
    if (std::fabs(det) < 1e-8f) return false; // ray parallel to triangle
    const float invDet = 1.0f / det;
    const Vector3D s = orig - v0;
    const float u = Dot(s, p) * invDet;
    if (u < 0.0f || u > 1.0f) return false;
    const Vector3D q = Cross(s, e1);
    const float v = Dot(dir, q) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;
    const float t = Dot(e2, q) * invDet;
    if (t < 0.0f) return false;
    tOut = t;
    Vector3D fn = Cross(e1, e2);
    const float fm = std::sqrt(Dot(fn, fn));
    nOut = (fm > 1e-12f) ? fn * (1.0f / fm) : Vector3D(0, 1, 0);
    return true;
}

} // namespace

// triangle-mesh raycast via BVH front-to-back traversal +
// Möller-Trumbore. Follows the legacy Collider raycast convention: applies
// the stored `position_` as translation (no rotation) - pose-correct mesh
// raycast would require body pose access, which the legacy API does not
// supply (consistent with SphereCollider/BoxCollider).
bool MeshCollider::Raycast(const Ray& ray, RaycastHit& hit, float maxDistance) {
    if (!data_ || data_->Empty()) return false;

    // Bring ray into mesh-local: subtract collider position. (Identity rotation.)
    const Vector3D origLocal(ray.origin.X - position_.X,
                             ray.origin.Y - position_.Y,
                             ray.origin.Z - position_.Z);
    const Vector3D dirLocal = ray.direction;

    float bestT = (maxDistance > 0.0f) ? maxDistance : std::numeric_limits<float>::max();
    bool  anyHit = false;
    Vector3D bestNormal(0, 1, 0);
    std::uint32_t bestTri = 0;

    data_->bvh.Raycast(origLocal, dirLocal, bestT,
        [&](std::uint32_t triIdx, float& /*tBestRef*/) -> bool {
            const std::uint32_t i0 = data_->indices[3 * triIdx + 0];
            const std::uint32_t i1 = data_->indices[3 * triIdx + 1];
            const std::uint32_t i2 = data_->indices[3 * triIdx + 2];
            const Vector3D& v0 = data_->vertices[i0];
            const Vector3D& v1 = data_->vertices[i1];
            const Vector3D& v2 = data_->vertices[i2];
            float t;
            Vector3D n;
            if (!RayTriangle(origLocal, dirLocal, v0, v1, v2, t, n)) return false;
            if (t >= bestT) return false;
            bestT      = t;
            bestNormal = n;
            bestTri    = triIdx;
            anyHit     = true;
            return true;
        });

    if (!anyHit) return false;

    // Double-sided: flip face normal to oppose ray direction.
    if (Dot(bestNormal, dirLocal) > 0.0f) bestNormal = bestNormal * -1.0f;

    hit.distance = bestT;
    hit.point    = Vector3D(ray.origin.X + dirLocal.X * bestT,
                            ray.origin.Y + dirLocal.Y * bestT,
                            ray.origin.Z + dirLocal.Z * bestT);
    hit.normal   = bestNormal; // identity rotation in legacy path
    hit.collider = this;
    (void)bestTri;
    return true;
}

} // namespace koilo

