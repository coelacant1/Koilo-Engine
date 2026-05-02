// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file heightfieldcollider.hpp
 * @brief Static heightfield collider.
 *
 * Wraps a shared HeightfieldData (regular height-grid) so multiple
 * HeightfieldColliders (e.g. instanced terrain tiles) can share one sample
 * array. Static-only: intended to live on a Static-mass body whose pose
 * places the heightfield in the world.
 *
 * ContainsPoint / ClosestPoint / Raycast follow the legacy collider API
 * (translation-only via stored `position_`, identity rotation), consistent
 * with SphereCollider / BoxCollider / MeshCollider; pose-correct queries go
 * through PhysicsWorld's narrowphase.
 */

#pragma once

#include <koilo/systems/physics/collider.hpp>
#include <koilo/systems/physics/shape/heightfielddata.hpp>
#include <koilo/registry/reflect_macros.hpp>

#include <memory>

namespace koilo {

class HeightfieldCollider : public Collider {
public:
    HeightfieldCollider();
    explicit HeightfieldCollider(std::shared_ptr<const HeightfieldData> data);
    ~HeightfieldCollider() override;

    /** Shared heightfield data. May be null until SetData() is called. */
    const std::shared_ptr<const HeightfieldData>& GetData() const { return data_; }
    void SetData(std::shared_ptr<const HeightfieldData> data) { data_ = std::move(data); }

    bool Raycast(const Ray& /*ray*/, RaycastHit& /*hit*/, float /*maxDistance*/) override {
        return false; // Heightfield raycast deferred (DDA path); narrowphase covers contact.
    }

    bool ContainsPoint(const Vector3D& /*point*/) override {
        return false; // Open-surface containment is undefined.
    }

    Vector3D ClosestPoint(const Vector3D& point) override {
        return point; // Not implemented in 7c; safe identity for legacy callers.
    }

    Vector3D GetPosition() const override { return position_; }
    void SetPosition(const Vector3D& pos) override { position_ = pos; }

    KL_BEGIN_FIELDS(HeightfieldCollider)
    KL_END_FIELDS

    KL_BEGIN_METHODS(HeightfieldCollider)
        KL_METHOD_AUTO(HeightfieldCollider, GetPosition, "Get position"),
        KL_METHOD_AUTO(HeightfieldCollider, SetPosition, "Set position")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(HeightfieldCollider)
        KL_CTOR0(HeightfieldCollider)
    KL_END_DESCRIBE(HeightfieldCollider)

private:
    std::shared_ptr<const HeightfieldData> data_;
    Vector3D position_{0, 0, 0};
};

} // namespace koilo
