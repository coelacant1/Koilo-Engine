// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file joint_internal.hpp
 * @brief shared helpers for joint Jacobian construction.
 *
 * Inline-only: kept out of the public API surface.
 */

#pragma once

#include "joint.hpp"

namespace koilo {
namespace joint_internal {

/**
 * @brief Build a point-coincidence linear constraint row along world `axis`.
 *
 * Constrains the world-space velocity component along `axis` of (anchor A
 * relative to anchor B) toward zero, with Baumgarte bias proportional to
 * the projected position error `posError = (worldB - worldA) · axis`.
 *
 * `rA`, `rB` are the world-space anchor offsets (rotated body-local
 * anchors). `axis` must be unit length.
 */
inline void BuildLinearRow(JointRow& row, const Vector3D& axis,
                           const Vector3D& rA, const Vector3D& rB,
                           float posError, const JointBuildContext& ctx,
                           float warmStart) {
    row.linA = axis * -1.0f;
    row.linB = axis;
    row.angA = rA.CrossProduct(axis) * -1.0f;
    row.angB = rB.CrossProduct(axis);

    float denom = 0.0f;
    if (ctx.dynamicA) {
        denom += ctx.invMassA;
        const Vector3D ka = ctx.invInertiaA.Multiply(rA.CrossProduct(axis)).CrossProduct(rA);
        denom += axis.DotProduct(ka);
    }
    if (ctx.dynamicB) {
        denom += ctx.invMassB;
        const Vector3D kb = ctx.invInertiaB.Multiply(rB.CrossProduct(axis)).CrossProduct(rB);
        denom += axis.DotProduct(kb);
    }
    row.effMass = (denom > 1.0e-12f) ? 1.0f / denom : 0.0f;
    row.bias = (ctx.dt > 0.0f) ? -(ctx.baumgarteBeta / ctx.dt) * posError : 0.0f;
    row.lowerImpulse = -1.0e30f;
    row.upperImpulse =  1.0e30f;
    row.accImpulse   = warmStart;
}

/**
 * @brief Build an angular alignment row along world `axis`.
 *
 * Constrains relative angular velocity component along `axis` toward zero
 * with Baumgarte bias from the orientation error component `angError`
 * (positive when wB relative to wA needs to subtract from `axis`).
 *
 * `axis` must be unit length. linA = linB = 0.
 */
inline void BuildAngularRow(JointRow& row, const Vector3D& axis, float angError,
                            const JointBuildContext& ctx, float warmStart) {
    row.linA = Vector3D(0,0,0);
    row.linB = Vector3D(0,0,0);
    row.angA = axis * -1.0f;
    row.angB = axis;

    float denom = 0.0f;
    if (ctx.dynamicA) {
        const Vector3D ia = ctx.invInertiaA.Multiply(axis);
        denom += axis.DotProduct(ia);
    }
    if (ctx.dynamicB) {
        const Vector3D ib = ctx.invInertiaB.Multiply(axis);
        denom += axis.DotProduct(ib);
    }
    row.effMass = (denom > 1.0e-12f) ? 1.0f / denom : 0.0f;
    row.bias = (ctx.dt > 0.0f) ? -(ctx.baumgarteBeta / ctx.dt) * angError : 0.0f;
    row.lowerImpulse = -1.0e30f;
    row.upperImpulse =  1.0e30f;
    row.accImpulse   = warmStart;
}

/**
 * @brief Compute the small-angle rotation error vector that takes the
 * current relative orientation `qA^-1 * qB` back to the rest pose `q0`.
 *
 * Returns `2 * q_err.xyz` rotated into world space (Featherstone convention),
 * where `q_err = qA * q0 * qB^-1` is the residual orientation error.
 * For small errors this approaches the world-space rotation vector that
 * would zero the offset.
 */
inline Vector3D OrientationErrorWorld(const Quaternion& qA,
                                      const Quaternion& qB,
                                      const Quaternion& restRel /* qA0^-1 * qB0 */) {
    // current relative: qRel = qA^-1 * qB; want qRel == restRel
    // err quaternion eRel = qRel * restRel^-1 (in A's frame)
    // Equivalent: err in world = qA * eRel * qA^-1
    const Quaternion qRel = qA.Conjugate().Multiply(qB);
    Quaternion err = qRel.Multiply(restRel.Conjugate());
    if (err.W < 0.0f) {  // shortest path
        err.W = -err.W; err.X = -err.X; err.Y = -err.Y; err.Z = -err.Z;
    }
    const Vector3D errA(2.0f * err.X, 2.0f * err.Y, 2.0f * err.Z);
    return qA.RotateVector(errA);
}

/**
 * @brief Pick two unit vectors orthogonal to `axis` (must be unit length)
 * forming a right-handed basis (axis, p1, p2).
 */
inline void PerpendicularBasis(const Vector3D& axis, Vector3D& p1, Vector3D& p2) {
    // Pick the world basis least aligned with axis.
    Vector3D ref = (std::fabs(axis.X) < 0.9f) ? Vector3D(1,0,0) : Vector3D(0,1,0);
    p1 = axis.CrossProduct(ref);
    const float len1 = p1.Magnitude();
    if (len1 > 1.0e-6f) p1 = p1 / len1;
    else                p1 = Vector3D(0,1,0);
    p2 = axis.CrossProduct(p1);
}

} // namespace joint_internal
} // namespace koilo
