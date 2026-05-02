// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file sphereshape.hpp
 * @brief Sphere shape implementation of IShape.
 */

#pragma once

#include "ishape.hpp"
#include <cmath>

namespace koilo {

class SphereShape final : public IShape {
public:
    float radius;

    SphereShape() : radius(0.5f) {}
    explicit SphereShape(float r) : radius(r) {}

    ShapeType Type() const override { return ShapeType::Sphere; }

    AABB LocalAABB() const override {
        return AABB(Vector3D(-radius,-radius,-radius), Vector3D(radius,radius,radius));
    }

    AABB WorldAABB(const BodyPose& pose) const override {
        const Vector3D r(radius, radius, radius);
        return AABB(pose.position - r, pose.position + r);
    }

    Vector3D Support(const Vector3D& dirWorld, const BodyPose& pose) const override {
        const float mag = dirWorld.Magnitude();
        if (mag < 1e-12f) return pose.position;
        const float inv = radius / mag;
        return pose.position + dirWorld * inv;
    }
};

} // namespace koilo
