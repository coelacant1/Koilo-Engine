// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file bodypose.hpp
 * @brief Pose (position + orientation) for rigid bodies and collider local offsets.
 *
 * Pose lives on the body, not on the collider geometry.
 * Used both as a body's world pose and as a collider's local offset relative
 * to its parent body.
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct BodyPose
 * @brief Position + orientation. Identity when default-constructed.
 */
struct BodyPose {
    Vector3D position;
    Quaternion orientation;

    BodyPose() : position(0.0f, 0.0f, 0.0f), orientation() {}
    BodyPose(const Vector3D& p, const Quaternion& q) : position(p), orientation(q) {}
    explicit BodyPose(const Vector3D& p) : position(p), orientation() {}

    /**
     * @brief Compose: this * child. child interpreted as offset in this's local frame.
     * Result world pos = this.position + this.orientation * child.position.
     * Result orientation = this.orientation * child.orientation.
     */
    BodyPose Compose(const BodyPose& child) const {
        BodyPose out;
        out.position = position + orientation.RotateVector(child.position);
        out.orientation = orientation * child.orientation;
        return out;
    }

    /**
     * @brief Linear interpolation of position + spherical interpolation of orientation.
     * @param a Start pose.
     * @param b End pose.
     * @param alpha [0,1].
     */
    static BodyPose Interpolate(const BodyPose& a, const BodyPose& b, float alpha) {
        BodyPose out;
        out.position = a.position + (b.position - a.position) * alpha;
        out.orientation = Quaternion::SphericalInterpolation(a.orientation, b.orientation, alpha);
        return out;
    }

    KL_BEGIN_FIELDS(BodyPose)
        KL_FIELD(BodyPose, position, "Position", 0, 0),
        KL_FIELD(BodyPose, orientation, "Orientation", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(BodyPose)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(BodyPose)
        KL_CTOR0(BodyPose),
        KL_CTOR(BodyPose, Vector3D)
    KL_END_DESCRIBE(BodyPose)
};

} // namespace koilo
