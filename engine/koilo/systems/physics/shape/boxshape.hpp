// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file boxshape.hpp
 * @brief Box (OBB) shape implementation of IShape.
 */

#pragma once

#include "ishape.hpp"
#include <koilo/core/geometry/3d/obb.hpp>
#include <cmath>

namespace koilo {

class BoxShape final : public IShape {
public:
    Vector3D halfExtents;

    BoxShape() : halfExtents(0.5f, 0.5f, 0.5f) {}
    explicit BoxShape(const Vector3D& he) : halfExtents(he) {}

    ShapeType Type() const override { return ShapeType::Box; }

    AABB LocalAABB() const override {
        return AABB(Vector3D(0,0,0) - halfExtents, halfExtents);
    }

    AABB WorldAABB(const BodyPose& pose) const override {
        OBB obb(pose.position, halfExtents, pose.orientation);
        return obb.EnclosingAABB();
    }

    Vector3D Support(const Vector3D& dirWorld, const BodyPose& pose) const override {
        const Vector3D dirLocal = pose.orientation.UnrotateVector(dirWorld);
        const Vector3D supportLocal(
            dirLocal.X >= 0.0f ?  halfExtents.X : -halfExtents.X,
            dirLocal.Y >= 0.0f ?  halfExtents.Y : -halfExtents.Y,
            dirLocal.Z >= 0.0f ?  halfExtents.Z : -halfExtents.Z
        );
        return pose.position + pose.orientation.RotateVector(supportLocal);
    }
};

} // namespace koilo
