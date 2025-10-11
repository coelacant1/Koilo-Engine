/**
 * @file raycasthit.hpp
 * @brief Raycast hit result information.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include "../../core/math/vector3d.hpp"
#include "../../registry/reflect_macros.hpp"

namespace ptx {

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

    PTX_BEGIN_FIELDS(RaycastHit)
        PTX_FIELD(RaycastHit, point, "Point", 0, 0),
        PTX_FIELD(RaycastHit, normal, "Normal", 0, 0),
        PTX_FIELD(RaycastHit, distance, "Distance", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(RaycastHit)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(RaycastHit)
        PTX_CTOR0(RaycastHit)
    PTX_END_DESCRIBE(RaycastHit)
};

} // namespace ptx
