// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file fixedjoint.hpp
 * @brief 6-DOF fully-rigid weld joint.
 *
 * Locks both relative position (3 lin rows) and relative orientation
 * (3 ang rows along world basis). The rest pose is captured at construction
 * time from the current relative orientation of the two bodies.
 */

#pragma once

#include "joint.hpp"
#include <koilo/core/math/quaternion.hpp>

namespace koilo {

class FixedJoint : public Joint {
public:
    FixedJoint(RigidBody* a, RigidBody* b,
               const Vector3D& localAnchorA, const Vector3D& localAnchorB);

    int GetTypeTag() const override { return static_cast<int>(JointType::Fixed); }
    int BuildRows(const JointBuildContext& ctx, JointRow* out) override;

    /**
     * @brief Re-capture the current relative orientation as the new rest pose.
     * Use after deliberate teleport/repositioning of either body to prevent
     * the joint from snapping bodies back to the original construction pose.
     * Also clears warm-start so stale impulses from the prior rest don't bias
     * the first post-recapture step.
     */
    void Recapture();

private:
    Vector3D   localAnchorA_;
    Vector3D   localAnchorB_;
    Quaternion restRel_;            ///< qA0^-1 * qB0 - rest relative orientation.
};

} // namespace koilo
