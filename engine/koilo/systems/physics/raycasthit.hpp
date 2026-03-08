// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file raycasthit.hpp
 * @brief Raycast hit result information.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

// Forward declaration
class Collider;

/**
 * @struct RaycastHit
 * @brief Contains information about a raycast hit.
 */
struct RaycastHit {
    Vector3D point;      ///< Hit point in world space
    Vector3D normal;     ///< Surface normal at hit point
    float distance;      ///< Distance from ray origin to hit point
    Collider* collider;  ///< The collider that was hit

    /**
     * @brief Default constructor.
     */
    RaycastHit()
        : point(0, 0, 0), normal(0, 1, 0), distance(0.0f), collider(nullptr) {}

    KL_BEGIN_FIELDS(RaycastHit)
        KL_FIELD(RaycastHit, point, "Point", 0, 0),
        KL_FIELD(RaycastHit, normal, "Normal", 0, 0),
        KL_FIELD(RaycastHit, distance, "Distance", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(RaycastHit)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(RaycastHit)
        KL_CTOR0(RaycastHit)
    KL_END_DESCRIBE(RaycastHit)
};

} // namespace koilo
