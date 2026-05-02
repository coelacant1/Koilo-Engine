// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ballsocketjoint.hpp
 * @brief 3-DOF point-coincidence joint (ball-and-socket).
 *
 * Locks the world-space positions of body-local anchors A and B together,
 * leaving relative orientation completely free. Solved as 3 linear rows
 * along world basis axes.
 */

#pragma once

#include "joint.hpp"

namespace koilo {

class BallSocketJoint : public Joint {
public:
    BallSocketJoint(RigidBody* a, RigidBody* b,
                    const Vector3D& localAnchorA, const Vector3D& localAnchorB);

    int GetTypeTag() const override { return static_cast<int>(JointType::BallSocket); }
    int BuildRows(const JointBuildContext& ctx, JointRow* out) override;

    Vector3D GetLocalAnchorA() const { return localAnchorA_; }
    Vector3D GetLocalAnchorB() const { return localAnchorB_; }

private:
    Vector3D localAnchorA_;
    Vector3D localAnchorB_;
};

} // namespace koilo
