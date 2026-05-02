// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file trianglemeshshape.hpp
 * @brief IShape wrapper around a shared TriangleMeshData.
 *
 * Concave (IsConvex() = false). Support() is a no-op; all queries go through
 * the BVH via QueryTriangles(). LocalAABB is the mesh's union AABB; WorldAABB
 * transforms its 8 corners by pose so rotated meshes get a tight enough box
 * for the broadphase.
 */

#pragma once

#include <koilo/systems/physics/shape/ishape.hpp>
#include <koilo/systems/physics/shape/trianglemeshdata.hpp>

#include <memory>

namespace koilo {

class TriangleMeshShape final : public IShape {
public:
    explicit TriangleMeshShape(std::shared_ptr<const TriangleMeshData> data)
        : data_(std::move(data)) {}

    const TriangleMeshData* Data() const { return data_.get(); }

    ShapeType Type() const override { return ShapeType::TriangleMesh; }
    bool IsConvex() const override { return false; }

    AABB LocalAABB() const override {
        return data_ ? data_->localAabb : AABB();
    }

    AABB WorldAABB(const BodyPose& pose) const override {
        if (!data_) return AABB(pose.position, pose.position);
        const AABB& l = data_->localAabb;
        // Transform the 8 local corners and take min/max.
        const Vector3D corners[8] = {
            Vector3D(l.min.X, l.min.Y, l.min.Z),
            Vector3D(l.max.X, l.min.Y, l.min.Z),
            Vector3D(l.min.X, l.max.Y, l.min.Z),
            Vector3D(l.max.X, l.max.Y, l.min.Z),
            Vector3D(l.min.X, l.min.Y, l.max.Z),
            Vector3D(l.max.X, l.min.Y, l.max.Z),
            Vector3D(l.min.X, l.max.Y, l.max.Z),
            Vector3D(l.max.X, l.max.Y, l.max.Z),
        };
        const Vector3D w0 = pose.position + pose.orientation.RotateVector(corners[0]);
        AABB out(w0, w0);
        for (int i = 1; i < 8; ++i) {
            out.Encapsulate(pose.position + pose.orientation.RotateVector(corners[i]));
        }
        return out;
    }

    Vector3D Support(const Vector3D&, const BodyPose& pose) const override {
        // Concave: not used by GJK/EPA. Return body origin as a safe stub.
        return pose.position;
    }

private:
    std::shared_ptr<const TriangleMeshData> data_;
};

} // namespace koilo
