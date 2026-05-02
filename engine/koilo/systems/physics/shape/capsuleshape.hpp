// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file capsuleshape.hpp
 * @brief Capsule shape implementation of IShape.
 *
 * Local frame: segment runs along local +Y/-Y from -halfHeight to +halfHeight.
 */

#pragma once

#include "ishape.hpp"
#include <cmath>

namespace koilo {

class CapsuleShape final : public IShape {
public:
    float radius;
    float halfHeight; ///< Half the length of the central segment (excludes hemisphere caps).

    CapsuleShape() : radius(0.5f), halfHeight(0.5f) {}
    CapsuleShape(float r, float hh) : radius(r), halfHeight(hh) {}

    ShapeType Type() const override { return ShapeType::Capsule; }

    AABB LocalAABB() const override {
        return AABB(Vector3D(-radius, -halfHeight - radius, -radius),
                    Vector3D( radius,  halfHeight + radius,  radius));
    }

    AABB WorldAABB(const BodyPose& pose) const override {
        const Vector3D axisLocal(0.0f, halfHeight, 0.0f);
        const Vector3D axisWorld = pose.orientation.RotateVector(axisLocal);
        const Vector3D pTop = pose.position + axisWorld;
        const Vector3D pBot = pose.position - axisWorld;
        Vector3D mn(std::min(pTop.X, pBot.X), std::min(pTop.Y, pBot.Y), std::min(pTop.Z, pBot.Z));
        Vector3D mx(std::max(pTop.X, pBot.X), std::max(pTop.Y, pBot.Y), std::max(pTop.Z, pBot.Z));
        const Vector3D r(radius, radius, radius);
        return AABB(mn - r, mx + r);
    }

    Vector3D Support(const Vector3D& dirWorld, const BodyPose& pose) const override {
        const Vector3D dirLocal = pose.orientation.UnrotateVector(dirWorld);
        const float yPick = (dirLocal.Y >= 0.0f) ? halfHeight : -halfHeight;
        Vector3D p(0.0f, yPick, 0.0f);
        const float mag = dirLocal.Magnitude();
        if (mag > 1e-12f) {
            p = p + dirLocal * (radius / mag);
        }
        return pose.position + pose.orientation.RotateVector(p);
    }
};

} // namespace koilo
