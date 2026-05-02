// SPDX-License-Identifier: GPL-3.0-or-later
#include "ballsocketjoint.hpp"
#include "joint_internal.hpp"

#include <koilo/systems/physics/rigidbody.hpp>

namespace koilo {

BallSocketJoint::BallSocketJoint(RigidBody* a, RigidBody* b,
                                 const Vector3D& localAnchorA, const Vector3D& localAnchorB)
    : Joint(a, b), localAnchorA_(localAnchorA), localAnchorB_(localAnchorB) {}

int BallSocketJoint::BuildRows(const JointBuildContext& ctx, JointRow* out) {
    if (!ctx.dynamicA && !ctx.dynamicB) return 0;

    const Vector3D rA = ctx.poseA.orientation.RotateVector(localAnchorA_);
    const Vector3D rB = ctx.poseB.orientation.RotateVector(localAnchorB_);
    const Vector3D worldA = ctx.poseA.position + rA;
    const Vector3D worldB = ctx.poseB.position + rB;
    const Vector3D delta  = worldB - worldA;

    using joint_internal::BuildLinearRow;
    static const Vector3D kBasis[3] = {
        Vector3D(1,0,0), Vector3D(0,1,0), Vector3D(0,0,1)
    };
    BuildLinearRow(out[0], kBasis[0], rA, rB, delta.X, ctx, accImpulseSlots_[0]);
    BuildLinearRow(out[1], kBasis[1], rA, rB, delta.Y, ctx, accImpulseSlots_[1]);
    BuildLinearRow(out[2], kBasis[2], rA, rB, delta.Z, ctx, accImpulseSlots_[2]);
    return 3;
}

} // namespace koilo
