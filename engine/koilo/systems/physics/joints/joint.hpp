// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file joint.hpp
 * @brief base class for rigid-body joints (constraints).
 *
 * Joints add constraint rows to the sequential-impulse solver alongside
 * contact rows. Each joint defines its own anchor frames in body-local
 * coordinates and rebuilds world-space Jacobians every solver step from
 * the current poses (so anchors survive arbitrary body motion).
 *
 * Determinism: joints are owned by the caller and added to PhysicsWorld in
 * insertion order. The solver iterates them in that order. T0/T1.
 *
 * Warm-start: each joint stores its own per-row accumulated impulse from
 * the previous step. The base class provides a fixed-size slot array
 * (`kMaxJointRows`) sized for the worst case (FixedJoint: 6 rows; Hinge
 * or Slider with both limit+motor active simultaneously: 7 rows).
 * Derived classes are responsible for clearing
 * stale slots when the active row count or row order changes.
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/matrix3x3.hpp>
#include <koilo/systems/physics/bodypose.hpp>

#include <cstdint>

namespace koilo {

class RigidBody;

/**
 * @struct JointBuildContext
 * @brief Public, solver-agnostic snapshot of body state passed to
 * `Joint::BuildRows`. Avoids leaking solver-private structs.
 */
struct JointBuildContext {
    BodyPose  poseA;
    BodyPose  poseB;
    Vector3D  linVelA;
    Vector3D  linVelB;
    Vector3D  angVelA;
    Vector3D  angVelB;
    float     invMassA = 0.0f;
    float     invMassB = 0.0f;
    Matrix3x3 invInertiaA;
    Matrix3x3 invInertiaB;
    bool      dynamicA = false;     ///< false => treat as immovable anchor
    bool      dynamicB = false;
    float     dt = 0.0f;
    float     baumgarteBeta = 0.1f; ///< Joint-specific Baumgarte coefficient.
};

/**
 * @struct JointRow
 * @brief One Featherstone-style velocity-constraint row.
 *
 * Constraint: `J · v + bias >= 0` (or `==` when both lo/hi finite).
 * Where `J = [linA angA linB angB]` and `v = [vA wA vB wB]`. After each
 * iteration, `lambda = effMass * (-Jv + bias)` is clamped cumulatively to
 * `[lowerImpulse, upperImpulse]` and applied as an impulse.
 */
struct JointRow {
    Vector3D linA;          ///< Linear Jacobian on body A.
    Vector3D angA;          ///< Angular Jacobian on body A.
    Vector3D linB;          ///< Linear Jacobian on body B (typically -linA).
    Vector3D angB;          ///< Angular Jacobian on body B.
    float    effMass = 0.0f;
    float    bias = 0.0f;
    float    lowerImpulse = -1.0e30f;   ///< Cumulative impulse lower bound.
    float    upperImpulse =  1.0e30f;   ///< Cumulative impulse upper bound.
    float    accImpulse = 0.0f;         ///< Warm-start input + iteration accumulator.
};

/**
 * @class Joint
 * @brief Base class for all joint types.
 */
class Joint {
public:
    static constexpr int kMaxJointRows = 7;

    Joint(RigidBody* a, RigidBody* b) : bodyA_(a), bodyB_(b) {}
    virtual ~Joint() = default;

    RigidBody* GetBodyA() const { return bodyA_; }
    RigidBody* GetBodyB() const { return bodyB_; }

    /**
     * @brief Joint type tag for diagnostics + warm-start invalidation.
     * Derived classes return a stable enum value.
     */
    virtual int GetTypeTag() const = 0;

    /**
     * @brief Populate up to kMaxJointRows constraint rows for this joint.
     * The implementation must precompute Jacobians, effective mass, bias,
     * and lower/upper impulse bounds. Warm-start impulses should be copied
     * from `accImpulseSlots_` into each row's `accImpulse` field; the
     * implementation should clamp them to the new bounds (and zero them
     * if the row regime/direction changed since last step) per the
     * warm-start invalidation rules.
     *
     * @param ctx  Body state snapshot.
     * @param out  Output array sized kMaxJointRows.
     * @return Number of rows written (0..kMaxJointRows). 0 = joint inactive
     *         (e.g. soft-disabled or both bodies static).
     */
    virtual int BuildRows(const JointBuildContext& ctx, JointRow* out) = 0;

    /**
     * @brief After the solver finishes velocity iterations, copy each
     * row's final `accImpulse` back into the joint for next-frame warm-start.
     * The order/count must match the most recent `BuildRows` call.
     */
    virtual void StoreImpulses(const JointRow* rows, int count) {
        for (int i = 0; i < count && i < kMaxJointRows; ++i) {
            accImpulseSlots_[i] = rows[i].accImpulse;
        }
        lastRowCount_ = count;
    }

    void ClearWarmStart() {
        for (int i = 0; i < kMaxJointRows; ++i) accImpulseSlots_[i] = 0.0f;
        lastRowCount_ = 0;
    }

protected:
    RigidBody* bodyA_;
    RigidBody* bodyB_;
    float      accImpulseSlots_[kMaxJointRows] = {0,0,0,0,0,0,0};
    int        lastRowCount_ = 0;
};

enum class JointType : int {
    Distance   = 0,
    BallSocket = 1,
    Fixed      = 2,
    Hinge      = 3,
    Slider     = 4,
};

} // namespace koilo
