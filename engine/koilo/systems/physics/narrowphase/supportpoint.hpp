// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file supportpoint.hpp
 * @brief Enriched Minkowski-difference vertex used by GJK and EPA.
 *
 * Carries the world-space support point on each shape so EPA can recover
 * proper witness points from face barycentrics. `featureA` / `featureB` are
 * placeholders for shape-specific feature ids; populated when shapes start
 * exposing that data.
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <cstdint>

namespace koilo {

class IShape;
class BodyPose;

struct SupportPoint {
    Vector3D point;       ///< supportA - supportB
    Vector3D supportA;    ///< support of A in +dir
    Vector3D supportB;    ///< support of B in -dir
    std::uint32_t featureA = 0;
    std::uint32_t featureB = 0;
};

} // namespace koilo
