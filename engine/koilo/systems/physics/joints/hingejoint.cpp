// SPDX-License-Identifier: GPL-3.0-or-later
#include "hingejoint.hpp"
#include "joint_internal.hpp"

#include <koilo/systems/physics/rigidbody.hpp>

#include <cmath>

namespace koilo {

HingeJoint::HingeJoint(RigidBody* a, RigidBody* b,
                       const Vector3D& localAnchorA, const Vector3D& localAnchorB,
                       const Vector3D& axisALocal, const Vector3D& axisBLocal,
                       const Vector3D& refALocal,  const Vector3D& refBLocal)
    : Joint(a, b),
      localAnchorA_(localAnchorA), localAnchorB_(localAnchorB),
      axisALocal_(axisALocal), axisBLocal_(axisBLocal),
      refALocal_(refALocal), refBLocal_(refBLocal) {}

int HingeJoint::BuildRows(const JointBuildContext& ctx, JointRow* out) {
    if (!ctx.dynamicA && !ctx.dynamicB) return 0;

    using joint_internal::BuildLinearRow;
    using joint_internal::BuildAngularRow;
    using joint_internal::PerpendicularBasis;

    // === Linear: 3 anchor coincidence rows. ===
    const Vector3D rA = ctx.poseA.orientation.RotateVector(localAnchorA_);
    const Vector3D rB = ctx.poseB.orientation.RotateVector(localAnchorB_);
    const Vector3D worldA = ctx.poseA.position + rA;
    const Vector3D worldB = ctx.poseB.position + rB;
    const Vector3D delta  = worldB - worldA;

    static const Vector3D kBasis[3] = {
        Vector3D(1,0,0), Vector3D(0,1,0), Vector3D(0,0,1)
    };
    BuildLinearRow(out[0], kBasis[0], rA, rB, delta.X, ctx, accImpulseSlots_[0]);
    BuildLinearRow(out[1], kBasis[1], rA, rB, delta.Y, ctx, accImpulseSlots_[1]);
    BuildLinearRow(out[2], kBasis[2], rA, rB, delta.Z, ctx, accImpulseSlots_[2]);

    // === Angular alignment: 2 perpendicular rows. ===
    const Vector3D axisAw = ctx.poseA.orientation.RotateVector(axisALocal_);
    const Vector3D axisBw = ctx.poseB.orientation.RotateVector(axisBLocal_);
    Vector3D p1, p2;
    PerpendicularBasis(axisAw, p1, p2);
    // Misalignment as small-angle rotation vector ≈ axisAw × axisBw.
    const Vector3D mis = axisAw.CrossProduct(axisBw);
    // Skip warm-start for these rows: perpendicular basis can rotate/flip.
    BuildAngularRow(out[3], p1, mis.DotProduct(p1), ctx, 0.0f);
    BuildAngularRow(out[4], p2, mis.DotProduct(p2), ctx, 0.0f);
    int rowCount = 5;

    // === Hinge angle (for limit + motor). ===
    // Project refA and refB onto plane perpendicular to axisAw.
    const Vector3D refAw = ctx.poseA.orientation.RotateVector(refALocal_);
    const Vector3D refBw = ctx.poseB.orientation.RotateVector(refBLocal_);
    auto projectPerp = [&](const Vector3D& v) {
        return v - axisAw * v.DotProduct(axisAw);
    };
    Vector3D pA = projectPerp(refAw);
    Vector3D pB = projectPerp(refBw);
    const float lpA = pA.Magnitude();
    const float lpB = pB.Magnitude();
    if (lpA > 1.0e-6f) pA = pA / lpA;
    if (lpB > 1.0e-6f) pB = pB / lpB;
    const float cosA = pA.DotProduct(pB);
    const float sinA = pA.CrossProduct(pB).DotProduct(axisAw);
    lastAngle_ = std::atan2(sinA, cosA);

    // === Limit row (single one-sided row when violated). ===
    if (limitEnabled_) {
        if (lastAngle_ < lowerLimit_) {
            // angVel along +axis must push angle up: bias > 0.
            const float angErr = lastAngle_ - lowerLimit_;  // negative
            BuildAngularRow(out[rowCount], axisAw, angErr, ctx, 0.0f);
            // Allow only positive impulses (push angle up).
            out[rowCount].lowerImpulse = 0.0f;
            out[rowCount].upperImpulse = 1.0e30f;
            ++rowCount;
        } else if (lastAngle_ > upperLimit_) {
            const float angErr = lastAngle_ - upperLimit_;  // positive
            BuildAngularRow(out[rowCount], axisAw, angErr, ctx, 0.0f);
            out[rowCount].lowerImpulse = -1.0e30f;
            out[rowCount].upperImpulse = 0.0f;
            ++rowCount;
        }
    }

    // === Motor row. ===
    if (motorEnabled_ && maxMotorTorque_ > 0.0f && rowCount < kMaxJointRows) {
        // Constraint: (wB - wA) · axis = motorSpeed_. Solver drives Jv -> bias,
        // so set bias = motorSpeed_ directly.
        BuildAngularRow(out[rowCount], axisAw, 0.0f, ctx, 0.0f);
        out[rowCount].bias = motorSpeed_;
        const float maxImp = maxMotorTorque_ * ctx.dt;
        out[rowCount].lowerImpulse = -maxImp;
        out[rowCount].upperImpulse =  maxImp;
        ++rowCount;
    }

    return rowCount;
}

} // namespace koilo
