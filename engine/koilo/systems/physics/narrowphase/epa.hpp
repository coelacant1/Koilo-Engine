// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file epa.hpp
 * @brief Expanding Polytope Algorithm - penetration depth + normal + witness pair.
 *
 * Consumes a GJK terminal simplex (typically a tetrahedron containing the
 * origin in Minkowski-difference space) and grows a convex polytope outward
 * until the closest face stabilizes. Witness points are recovered from the
 * barycentric coordinates of the projection of the origin onto that face,
 * applied separately to each shape's stored support point.
 */

#pragma once

#include "supportpoint.hpp"
#include "gjk.hpp"
#include <koilo/core/math/vector3d.hpp>

namespace koilo {

struct EpaResult {
    bool      ok = false;
    Vector3D  normal{0,0,0};      ///< Unit normal pointing from B toward A (separation direction).
    float     depth = 0.0f;       ///< Penetration depth along normal.
    Vector3D  witnessA{0,0,0};    ///< Contact witness on shape A (world space).
    Vector3D  witnessB{0,0,0};    ///< Contact witness on shape B (world space).
    int       iterations = 0;
};

EpaResult Epa(const IShape& a, const BodyPose& poseA,
              const IShape& b, const BodyPose& poseB,
              const GjkResult& gjk,
              int maxIterations = 64,
              float tolerance = 1e-4f);

} // namespace koilo
