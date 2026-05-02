// SPDX-License-Identifier: GPL-3.0-or-later
#include "distancejoint.hpp"

#include <koilo/systems/physics/rigidbody.hpp>

#include <cmath>
#include <algorithm>

namespace koilo {

DistanceJoint::DistanceJoint(RigidBody* a, RigidBody* b,
                             const Vector3D& localAnchorA, const Vector3D& localAnchorB,
                             float targetLength)
    : Joint(a, b),
      localAnchorA_(localAnchorA), localAnchorB_(localAnchorB),
      targetLength_(targetLength) {
    if (targetLength_ < 0.0f && a && b) {
        const Vector3D wA = a->GetPose().position + a->GetPose().orientation.RotateVector(localAnchorA);
        const Vector3D wB = b->GetPose().position + b->GetPose().orientation.RotateVector(localAnchorB);
        targetLength_ = (wB - wA).Magnitude();
    }
    if (targetLength_ < 0.0f) targetLength_ = 0.0f;
}

int DistanceJoint::BuildRows(const JointBuildContext& ctx, JointRow* out) {
    if (!ctx.dynamicA && !ctx.dynamicB) return 0;

    const Vector3D rA = ctx.poseA.orientation.RotateVector(localAnchorA_);
    const Vector3D rB = ctx.poseB.orientation.RotateVector(localAnchorB_);
    const Vector3D worldA = ctx.poseA.position + rA;
    const Vector3D worldB = ctx.poseB.position + rB;

    Vector3D delta = worldB - worldA;
    float dist = delta.Magnitude();

    Vector3D u;
    if (dist > 1.0e-6f) {
        u = delta / dist;
    } else {
        // Degenerate: anchors coincide. Pick an arbitrary stable axis so the
        // row math doesn't NaN. Bias is zero (already at target if target=0,
        // or maximally compressed if target>0 - solver still applies pushes).
        u = Vector3D(1.0f, 0.0f, 0.0f);
    }

    JointRow& r = out[0];
    r.linA = u * -1.0f;                       // pulling B toward A => +impulse on B in +u, -u on A.
    r.linB = u;
    r.angA = rA.CrossProduct(u) * -1.0f;
    r.angB = rB.CrossProduct(u);

    // Effective mass = 1 / (J M^-1 J^T)
    float denom = 0.0f;
    if (ctx.dynamicA) {
        denom += ctx.invMassA;
        const Vector3D ka = ctx.invInertiaA.Multiply(rA.CrossProduct(u)).CrossProduct(rA);
        denom += u.DotProduct(ka);
    }
    if (ctx.dynamicB) {
        denom += ctx.invMassB;
        const Vector3D kb = ctx.invInertiaB.Multiply(rB.CrossProduct(u)).CrossProduct(rB);
        denom += u.DotProduct(kb);
    }
    r.effMass = (denom > 1.0e-12f) ? 1.0f / denom : 0.0f;

    // Bias: target post-impulse (Jv) = -beta/dt * positionError, where
    // positionError = (dist - targetLength). If stretched (>target), bias
    // pulls bodies together (negative velocity along +u from B's PoV).
    const float positionError = dist - targetLength_;
    r.bias = (ctx.dt > 0.0f) ? -(ctx.baumgarteBeta / ctx.dt) * positionError : 0.0f;

    r.lowerImpulse = -1.0e30f;
    r.upperImpulse =  1.0e30f;
    r.accImpulse   = accImpulseSlots_[0];     // warm-start (bounds are constant => no clamp needed)
    return 1;
}

} // namespace koilo
