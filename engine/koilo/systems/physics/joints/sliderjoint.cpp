// SPDX-License-Identifier: GPL-3.0-or-later
#include "sliderjoint.hpp"
#include "joint_internal.hpp"

#include <koilo/systems/physics/rigidbody.hpp>

#include <cmath>

namespace koilo {

SliderJoint::SliderJoint(RigidBody* a, RigidBody* b,
                         const Vector3D& localAnchorA, const Vector3D& localAnchorB,
                         const Vector3D& axisALocal)
    : Joint(a, b),
      localAnchorA_(localAnchorA), localAnchorB_(localAnchorB),
      axisALocal_(axisALocal) {
    if (a && b) {
        restRel_ = a->GetPose().orientation.Conjugate().Multiply(b->GetPose().orientation);
    } else {
        restRel_ = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
    }
}

int SliderJoint::BuildRows(const JointBuildContext& ctx, JointRow* out) {
    if (!ctx.dynamicA && !ctx.dynamicB) return 0;

    using joint_internal::BuildLinearRow;
    using joint_internal::BuildAngularRow;
    using joint_internal::PerpendicularBasis;
    using joint_internal::OrientationErrorWorld;

    const Vector3D rA = ctx.poseA.orientation.RotateVector(localAnchorA_);
    const Vector3D rB = ctx.poseB.orientation.RotateVector(localAnchorB_);
    const Vector3D worldA = ctx.poseA.position + rA;
    const Vector3D worldB = ctx.poseB.position + rB;
    const Vector3D delta  = worldB - worldA;

    // Slider axis in world: rotated by A.
    const Vector3D axisW = ctx.poseA.orientation.RotateVector(axisALocal_);
    Vector3D p1, p2;
    PerpendicularBasis(axisW, p1, p2);

    // === 2 linear perpendicular rows. ===
    // Skip warm-start: perpendicular basis can rotate as A rotates.
    BuildLinearRow(out[0], p1, rA, rB, delta.DotProduct(p1), ctx, 0.0f);
    BuildLinearRow(out[1], p2, rA, rB, delta.DotProduct(p2), ctx, 0.0f);

    // === 3 angular lock rows along world basis. ===
    static const Vector3D kBasis[3] = {
        Vector3D(1,0,0), Vector3D(0,1,0), Vector3D(0,0,1)
    };
    const Vector3D angErr = OrientationErrorWorld(
        ctx.poseA.orientation, ctx.poseB.orientation, restRel_);
    BuildAngularRow(out[2], kBasis[0], angErr.X, ctx, accImpulseSlots_[2]);
    BuildAngularRow(out[3], kBasis[1], angErr.Y, ctx, accImpulseSlots_[3]);
    BuildAngularRow(out[4], kBasis[2], angErr.Z, ctx, accImpulseSlots_[4]);

    int rowCount = 5;
    lastDisplacement_ = delta.DotProduct(axisW);

    // === Limit row along slider axis. ===
    if (limitEnabled_ && rowCount < kMaxJointRows) {
        if (lastDisplacement_ < lowerLimit_) {
            BuildLinearRow(out[rowCount], axisW, rA, rB,
                           lastDisplacement_ - lowerLimit_, ctx, 0.0f);
            out[rowCount].lowerImpulse = 0.0f;
            out[rowCount].upperImpulse = 1.0e30f;
            ++rowCount;
        } else if (lastDisplacement_ > upperLimit_) {
            BuildLinearRow(out[rowCount], axisW, rA, rB,
                           lastDisplacement_ - upperLimit_, ctx, 0.0f);
            out[rowCount].lowerImpulse = -1.0e30f;
            out[rowCount].upperImpulse = 0.0f;
            ++rowCount;
        }
    }

    // === Motor row along slider axis. ===
    if (motorEnabled_ && maxMotorForce_ > 0.0f && rowCount < kMaxJointRows) {
        BuildLinearRow(out[rowCount], axisW, rA, rB, 0.0f, ctx, 0.0f);
        out[rowCount].bias = motorSpeed_;
        const float maxImp = maxMotorForce_ * ctx.dt;
        out[rowCount].lowerImpulse = -maxImp;
        out[rowCount].upperImpulse =  maxImp;
        ++rowCount;
    }

    return rowCount;
}

} // namespace koilo
