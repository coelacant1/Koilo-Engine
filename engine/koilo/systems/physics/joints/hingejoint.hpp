// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file hingejoint.hpp
 * @brief 1-DOF revolute joint about a body-local axis.
 *
 * Constraint rows:
 *   - 3 linear coincidence rows at the anchor (always active).
 *   - 2 angular rows perpendicular to the hinge axis (always active).
 *   - 1 angular limit row (active only when angle is outside [lo,hi]).
 *   - 1 angular motor row (active when motor is enabled, sized by max torque).
 *
 * Reference angle is measured between body-local reference vectors `refA`
 * and `refB` (both perpendicular to their respective hinge axes), projected
 * onto the world plane perpendicular to the current hinge axis.
 */

#pragma once

#include "joint.hpp"

namespace koilo {

class HingeJoint : public Joint {
public:
    /**
     * @param axisALocal Hinge axis in body A's local frame (unit length).
     * @param axisBLocal Hinge axis in body B's local frame (unit length;
     *                   should align with axisALocal in world space at rest).
     * @param refALocal  Body A reference vector, perpendicular to axisALocal;
     *                   used as the zero-angle direction.
     * @param refBLocal  Body B reference vector, perpendicular to axisBLocal.
     */
    HingeJoint(RigidBody* a, RigidBody* b,
               const Vector3D& localAnchorA, const Vector3D& localAnchorB,
               const Vector3D& axisALocal, const Vector3D& axisBLocal,
               const Vector3D& refALocal,  const Vector3D& refBLocal);

    int GetTypeTag() const override { return static_cast<int>(JointType::Hinge); }
    int BuildRows(const JointBuildContext& ctx, JointRow* out) override;

    // === Limits ===
    void EnableLimit(bool enabled, float lowerRadians, float upperRadians) {
        limitEnabled_ = enabled;
        lowerLimit_ = lowerRadians;
        upperLimit_ = upperRadians;
    }
    bool IsLimitEnabled() const { return limitEnabled_; }

    // === Motor ===
    void EnableMotor(bool enabled, float targetAngVel, float maxTorque) {
        motorEnabled_ = enabled;
        motorSpeed_   = targetAngVel;
        maxMotorTorque_ = maxTorque;
    }
    bool IsMotorEnabled() const { return motorEnabled_; }

    /** @brief Last computed hinge angle (radians) from previous BuildRows. */
    float GetCurrentAngle() const { return lastAngle_; }

private:
    Vector3D localAnchorA_, localAnchorB_;
    Vector3D axisALocal_,   axisBLocal_;
    Vector3D refALocal_,    refBLocal_;

    bool  limitEnabled_ = false;
    float lowerLimit_   = 0.0f;
    float upperLimit_   = 0.0f;

    bool  motorEnabled_   = false;
    float motorSpeed_     = 0.0f;
    float maxMotorTorque_ = 0.0f;

    float lastAngle_ = 0.0f;
};

} // namespace koilo
