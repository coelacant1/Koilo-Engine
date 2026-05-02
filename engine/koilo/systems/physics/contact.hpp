// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file contact.hpp
 * @brief Single contact point between two colliders.
 *
 * `featureId` is a stable hash of the contributing geometric features (e.g.
 * vertex/edge/face indices for a hull, "0" for a sphere). It is the key used
 * by `ContactCache` to carry warm-start impulses across frames.
 *
 * `accumulatedNormalImpulse` and `accumulatedTangentImpulse[2]` are populated
 * by the sequential-impulse solver.
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <cstdint>

namespace koilo {

struct Contact {
    Vector3D point;                          ///< World-space contact point.
    Vector3D normal;                         ///< Unit normal pointing from B->A (push direction for body A out of body B).
    float depth;                             ///< Penetration depth (≥ 0).
    std::uint64_t featureId;                 ///< Stable feature pair identifier.
    float accumulatedNormalImpulse;          ///< Warm-start: normal impulse from prior frame.
    float accumulatedTangentImpulse[2];      ///< Warm-start: friction impulses (2-tangent pyramid).

    Contact()
        : point(0, 0, 0), normal(0, 1, 0), depth(0.0f), featureId(0),
          accumulatedNormalImpulse(0.0f), accumulatedTangentImpulse{0.0f, 0.0f} {}

    Contact(const Vector3D& p, const Vector3D& n, float d, std::uint64_t feat)
        : point(p), normal(n), depth(d), featureId(feat),
          accumulatedNormalImpulse(0.0f), accumulatedTangentImpulse{0.0f, 0.0f} {}
};

} // namespace koilo
