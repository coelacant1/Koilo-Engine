// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixedjoint.hpp"
#include "joint_internal.hpp"

#include <koilo/systems/physics/rigidbody.hpp>

namespace koilo {

FixedJoint::FixedJoint(RigidBody* a, RigidBody* b,
                       const Vector3D& localAnchorA, const Vector3D& localAnchorB)
    : Joint(a, b), localAnchorA_(localAnchorA), localAnchorB_(localAnchorB) {
    if (a && b) {
        restRel_ = a->GetPose().orientation.Conjugate().Multiply(b->GetPose().orientation);
    } else {
        restRel_ = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
    }
}

int FixedJoint::BuildRows(const JointBuildContext& ctx, JointRow* out) {
    if (!ctx.dynamicA && !ctx.dynamicB) return 0;

    using joint_internal::BuildLinearRow;
    using joint_internal::BuildAngularRow;

    const Vector3D rA = ctx.poseA.orientation.RotateVector(localAnchorA_);
    const Vector3D rB = ctx.poseB.orientation.RotateVector(localAnchorB_);
    const Vector3D worldA = ctx.poseA.position + rA;
    const Vector3D worldB = ctx.poseB.position + rB;
    const Vector3D delta  = worldB - worldA;

    static const Vector3D kBasis[3] = {
        Vector3D(1,0,0), Vector3D(0,1,0), Vector3D(0,0,1)
    };

    // 3 linear coincidence rows.
    BuildLinearRow(out[0], kBasis[0], rA, rB, delta.X, ctx, accImpulseSlots_[0]);
    BuildLinearRow(out[1], kBasis[1], rA, rB, delta.Y, ctx, accImpulseSlots_[1]);
    BuildLinearRow(out[2], kBasis[2], rA, rB, delta.Z, ctx, accImpulseSlots_[2]);

    // 3 angular alignment rows in world frame.
    const Vector3D angErr = joint_internal::OrientationErrorWorld(
        ctx.poseA.orientation, ctx.poseB.orientation, restRel_);
    BuildAngularRow(out[3], kBasis[0], angErr.X, ctx, accImpulseSlots_[3]);
    BuildAngularRow(out[4], kBasis[1], angErr.Y, ctx, accImpulseSlots_[4]);
    BuildAngularRow(out[5], kBasis[2], angErr.Z, ctx, accImpulseSlots_[5]);
    return 6;
}

void FixedJoint::Recapture() {
    if (bodyA_ && bodyB_) {
        restRel_ = bodyA_->GetPose().orientation.Conjugate().Multiply(
            bodyB_->GetPose().orientation);
    } else {
        restRel_ = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
    }
    ClearWarmStart();
}

} // namespace koilo
