// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file sliderjoint.hpp
 * @brief 1-DOF prismatic joint along a body-A-local axis.
 *
 * Constraint rows:
 *   - 2 linear rows perpendicular to the slider axis (always active).
 *   - 3 angular rows locking orientation (always active, world basis).
 *   - 1 linear limit row along the axis (active when out of [lo,hi]).
 *   - 1 linear motor row along the axis (active when motor enabled).
 */

#pragma once

#include "joint.hpp"
#include <koilo/core/math/quaternion.hpp>

namespace koilo {

class SliderJoint : public Joint {
public:
    SliderJoint(RigidBody* a, RigidBody* b,
                const Vector3D& localAnchorA, const Vector3D& localAnchorB,
                const Vector3D& axisALocal);

    int GetTypeTag() const override { return static_cast<int>(JointType::Slider); }
    int BuildRows(const JointBuildContext& ctx, JointRow* out) override;

    void EnableLimit(bool enabled, float lower, float upper) {
        limitEnabled_ = enabled;
        lowerLimit_ = lower;
        upperLimit_ = upper;
    }

    void EnableMotor(bool enabled, float targetLinearVel, float maxForce) {
        motorEnabled_ = enabled;
        motorSpeed_ = targetLinearVel;
        maxMotorForce_ = maxForce;
    }

    /** @brief Last-built signed displacement along the slider axis (m). */
    float GetCurrentDisplacement() const { return lastDisplacement_; }

private:
    Vector3D   localAnchorA_, localAnchorB_;
    Vector3D   axisALocal_;
    Quaternion restRel_;            ///< qA0^-1 * qB0 - captured at construction.

    bool  limitEnabled_ = false;
    float lowerLimit_   = 0.0f;
    float upperLimit_   = 0.0f;

    bool  motorEnabled_   = false;
    float motorSpeed_     = 0.0f;
    float maxMotorForce_  = 0.0f;

    float lastDisplacement_ = 0.0f;
};

} // namespace koilo
