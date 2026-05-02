// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file convexhullshape.hpp
 * @brief Convex hull shape implementation of IShape.
 */

#pragma once

#include "ishape.hpp"
#include <koilo/core/geometry/3d/convexhull.hpp>
#include <cmath>

namespace koilo {

class ConvexHullShape final : public IShape {
public:
    ConvexHull hull;

    ConvexHullShape() = default;
    explicit ConvexHullShape(ConvexHull h) : hull(std::move(h)) {}

    ShapeType Type() const override { return ShapeType::ConvexHull; }

    AABB LocalAABB() const override { return hull.ComputeBounds(); }

    AABB WorldAABB(const BodyPose& pose) const override {
        if (hull.vertices.empty()) {
            return AABB(pose.position, pose.position);
        }
        Vector3D first = pose.position + pose.orientation.RotateVector(hull.vertices[0]);
        Vector3D mn = first, mx = first;
        for (std::size_t i = 1; i < hull.vertices.size(); ++i) {
            Vector3D w = pose.position + pose.orientation.RotateVector(hull.vertices[i]);
            if (w.X < mn.X) mn.X = w.X; else if (w.X > mx.X) mx.X = w.X;
            if (w.Y < mn.Y) mn.Y = w.Y; else if (w.Y > mx.Y) mx.Y = w.Y;
            if (w.Z < mn.Z) mn.Z = w.Z; else if (w.Z > mx.Z) mx.Z = w.Z;
        }
        return AABB(mn, mx);
    }

    Vector3D Support(const Vector3D& dirWorld, const BodyPose& pose) const override {
        const Vector3D dirLocal = pose.orientation.UnrotateVector(dirWorld);
        const Vector3D supportLocal = hull.Support(dirLocal);
        return pose.position + pose.orientation.RotateVector(supportLocal);
    }
};

} // namespace koilo
