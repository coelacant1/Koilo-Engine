// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file heightfieldshape.hpp
 * @brief IShape wrapper around a shared HeightfieldData.
 *
 * Concave (IsConvex() = false). Support() is a no-op; sphere/box/capsule
 * narrowphase iterate the cells overlapping the dynamic shape's local AABB.
 * LocalAABB is the heightfield's union AABB; WorldAABB transforms its 8
 * corners by pose so rotated terrain still gets a tight broadphase box.
 */

#pragma once

#include <koilo/systems/physics/shape/ishape.hpp>
#include <koilo/systems/physics/shape/heightfielddata.hpp>

#include <memory>

namespace koilo {

class HeightfieldShape final : public IShape {
public:
    explicit HeightfieldShape(std::shared_ptr<const HeightfieldData> data)
        : data_(std::move(data)) {}

    const HeightfieldData* Data() const { return data_.get(); }

    ShapeType Type() const override { return ShapeType::HeightField; }
    bool IsConvex() const override { return false; }

    AABB LocalAABB() const override {
        return data_ ? data_->localAabb : AABB();
    }

    AABB WorldAABB(const BodyPose& pose) const override {
        if (!data_ || data_->Empty()) return AABB(pose.position, pose.position);
        const AABB& l = data_->localAabb;
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
        return pose.position; // Concave: not used by GJK/EPA.
    }

private:
    std::shared_ptr<const HeightfieldData> data_;
};

} // namespace koilo
