// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file planeshape.hpp
 * @brief Infinite plane shape - non-convex from GJK's perspective.
 *
 * Plane lives in local space as { normal · p = distance }. Sphere-plane and
 * box-plane fast paths are special-cased; GJK is not invoked.
 */

#pragma once

#include "ishape.hpp"
#include <limits>

namespace koilo {

class PlaneShape final : public IShape {
public:
    Vector3D normalLocal;  ///< Unit normal in local space.
    float    distance;     ///< Signed distance from local origin along normal.

    PlaneShape() : normalLocal(0.0f, 1.0f, 0.0f), distance(0.0f) {}
    PlaneShape(const Vector3D& n, float d) : normalLocal(n), distance(d) {}

    ShapeType Type() const override { return ShapeType::Plane; }
    bool IsConvex() const override { return false; }

    AABB LocalAABB() const override {
        const float big = 1e18f;
        return AABB(Vector3D(-big,-big,-big), Vector3D(big,big,big));
    }

    AABB WorldAABB(const BodyPose& pose) const override {
        (void)pose;
        return LocalAABB();
    }

    /** GJK contract violation guard - planes are not convex per IShape. */
    Vector3D Support(const Vector3D& /*dirWorld*/, const BodyPose& pose) const override {
        return pose.position;
    }

    /** World-space plane normal. */
    Vector3D NormalWorld(const BodyPose& pose) const {
        return pose.orientation.RotateVector(normalLocal);
    }

    /** World-space signed offset along the world normal. */
    float DistanceWorld(const BodyPose& pose) const {
        return distance + NormalWorld(pose).DotProduct(pose.position);
    }
};

} // namespace koilo
