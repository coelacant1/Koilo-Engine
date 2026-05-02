// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file gjk.hpp
 * @brief Boolean / closest-point Gilbert-Johnson-Keerthi algorithm.
 *
 * Operates on the support-mapping interface (`IShape::Support`). Returns
 * either intersection (with the terminal simplex for EPA hand-off) or
 * separation (with witness points and distance).
 *
 * Hardening: hard iteration cap, duplicate-support termination, epsilon
 * progress check. No allocations on the hot path.
 */

#pragma once

#include "supportpoint.hpp"
#include <koilo/systems/physics/shape/ishape.hpp>
#include <koilo/systems/physics/bodypose.hpp>
#include <cstdint>

namespace koilo {

struct GjkResult {
    bool          intersect = false;
    int           iterations = 0;
    int           simplexCount = 0;          ///< Valid range [1..4]
    SupportPoint  simplex[4];                ///< Terminal simplex (origin enclosed if intersect=true)
    Vector3D      closestA{0,0,0};           ///< Witness on A (separation case)
    Vector3D      closestB{0,0,0};           ///< Witness on B (separation case)
    float         distance = 0.0f;           ///< |closestA - closestB|; 0 on intersection
};

/**
 * @brief Runs GJK between two convex shapes.
 *
 * Termination rules (in order):
 *   1. Origin enclosed by simplex -> intersect=true.
 *   2. New support point fails to advance toward origin (within epsilon) -> separation.
 *   3. Duplicate support point (within epsilon) -> separation.
 *   4. Iteration cap reached -> separation with current best.
 */
GjkResult Gjk(const IShape& a, const BodyPose& poseA,
              const IShape& b, const BodyPose& poseB,
              int maxIterations = 32);

/** Internal helper: signed support of the Minkowski difference in direction d. */
SupportPoint MinkowskiSupport(const IShape& a, const BodyPose& poseA,
                              const IShape& b, const BodyPose& poseB,
                              const Vector3D& d);

} // namespace koilo
