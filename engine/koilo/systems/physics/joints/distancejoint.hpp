// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file distancejoint.hpp
 * @brief keeps two body-local anchors at a fixed world-space distance.
 *
 * A single constraint row along the line between the two world-space
 * anchor points. Acts as a stiff massless rod (lower=upper=±inf) - pulls
 * the two bodies together when stretched and pushes apart when compressed.
 */

#pragma once

#include "joint.hpp"

namespace koilo {

class DistanceJoint : public Joint {
public:
    /**
     * @param a            Body A.
     * @param b            Body B (may be nullptr-safe via the world's static-anchor handling).
     * @param localAnchorA Anchor in A's local frame.
     * @param localAnchorB Anchor in B's local frame.
     * @param targetLength Target world-space distance between anchors. If <0,
     *                     the constructor records the current distance as the target.
     */
    DistanceJoint(RigidBody* a, RigidBody* b,
                  const Vector3D& localAnchorA, const Vector3D& localAnchorB,
                  float targetLength = -1.0f);

    int GetTypeTag() const override { return static_cast<int>(JointType::Distance); }
    int BuildRows(const JointBuildContext& ctx, JointRow* out) override;

    float  GetTargetLength() const { return targetLength_; }
    void   SetTargetLength(float l) { targetLength_ = l; }
    Vector3D GetLocalAnchorA() const { return localAnchorA_; }
    Vector3D GetLocalAnchorB() const { return localAnchorB_; }

private:
    Vector3D localAnchorA_;
    Vector3D localAnchorB_;
    float    targetLength_;
};

} // namespace koilo
