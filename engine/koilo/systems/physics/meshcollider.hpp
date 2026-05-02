// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file meshcollider.hpp
 * @brief Static triangle-mesh collider.
 *
 * Wraps a shared TriangleMeshData (vertices + indices + BVH) so multiple
 * MeshColliders (e.g. instanced level geometry) share one BVH. Static-only:
 * intended to live on a Static-mass body whose pose places the mesh in the
 * world. The manifold pipeline reads body pose; legacy Get/SetPosition are
 * tracked locally for back-compat with the CollisionManager raycast API.
 *
 * Raycast / ContainsPoint / ClosestPoint are not implemented in 7a - the
 * dynamic side queries the mesh through the narrowphase only.
 */

#pragma once

#include <koilo/systems/physics/collider.hpp>
#include <koilo/systems/physics/shape/trianglemeshdata.hpp>
#include <koilo/registry/reflect_macros.hpp>

#include <memory>

namespace koilo {

class MeshCollider : public Collider {
public:
    MeshCollider();
    explicit MeshCollider(std::shared_ptr<const TriangleMeshData> data);
    ~MeshCollider() override;

    /** Shared mesh data + BVH. May be null until SetData() is called. */
    const std::shared_ptr<const TriangleMeshData>& GetData() const { return data_; }
    void SetData(std::shared_ptr<const TriangleMeshData> data) { data_ = std::move(data); }

    // === Collider Interface ===

    bool Raycast(const Ray& ray, RaycastHit& hit, float maxDistance) override;

    bool ContainsPoint(const Vector3D& /*point*/) override {
        return false; // Open-mesh containment is undefined; static surface only.
    }

    Vector3D ClosestPoint(const Vector3D& point) override {
        return point; // Not implemented in 7a; safe identity for legacy callers.
    }

    Vector3D GetPosition() const override { return position_; }
    void SetPosition(const Vector3D& pos) override { position_ = pos; }

    KL_BEGIN_FIELDS(MeshCollider)
        // Inherits from Collider; no extra reflected fields in 7a.
    KL_END_FIELDS

    KL_BEGIN_METHODS(MeshCollider)
        KL_METHOD_AUTO(MeshCollider, GetPosition, "Get position"),
        KL_METHOD_AUTO(MeshCollider, SetPosition, "Set position")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MeshCollider)
        KL_CTOR0(MeshCollider)
    KL_END_DESCRIBE(MeshCollider)

private:
    std::shared_ptr<const TriangleMeshData> data_;
    Vector3D position_{0, 0, 0};
};

} // namespace koilo
