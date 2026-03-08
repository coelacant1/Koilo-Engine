// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file physicsraycast.hpp
 * @brief Physics raycasting utilities for collision detection.
 *
 * Provides high-level raycasting functions for physics queries, including
 * layer-based filtering, multiple hit detection, and collision manager integration.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <vector>
#include "collider.hpp"
#include "raycasthit.hpp"
#include <koilo/core/geometry/ray.hpp>
#include <koilo/core/math/mathematics.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class PhysicsRaycast
 * @brief Static utility class for physics raycasting operations.
 *
 * Provides centralized raycasting functionality for physics collision detection.
 * Works with collider lists and supports layer-based filtering.
 */
class PhysicsRaycast {
public:
    /**
     * @brief Performs a raycast against a list of colliders, finding the closest hit.
     * @param ray The ray to cast.
     * @param colliders Vector of colliders to test against.
     * @param hit Output hit information for closest hit.
     * @param maxDistance Maximum ray distance.
     * @param layerMask Layer mask for filtering (-1 = all layers).
     * @return True if any collider was hit.
     */
    static bool Raycast(const Ray& ray,
                       const std::vector<Collider*>& colliders,
                       RaycastHit& hit,
                       float maxDistance = Mathematics::FLTMAX,
                       int layerMask = -1);

    /**
     * @brief Performs a raycast finding all hits along the ray.
     * @param ray The ray to cast.
     * @param colliders Vector of colliders to test against.
     * @param maxDistance Maximum ray distance.
     * @param layerMask Layer mask for filtering (-1 = all layers).
     * @return Vector of all hits, sorted by distance (closest first).
     */
    static std::vector<RaycastHit> RaycastAll(const Ray& ray,
                                              const std::vector<Collider*>& colliders,
                                              float maxDistance = Mathematics::FLTMAX,
                                              int layerMask = -1);

    /**
     * @brief Checks if ray hits any collider (does not return hit info).
     * @param ray The ray to cast.
     * @param colliders Vector of colliders to test against.
     * @param maxDistance Maximum ray distance.
     * @param layerMask Layer mask for filtering (-1 = all layers).
     * @return True if any collider was hit.
     */
    static bool RaycastAny(const Ray& ray,
                          const std::vector<Collider*>& colliders,
                          float maxDistance = Mathematics::FLTMAX,
                          int layerMask = -1);

    /**
     * @brief Checks if a layer should be included in raycast based on mask.
     * @param layer Layer index (0-31).
     * @param layerMask Bitmask of layers to include (-1 = all).
     * @return True if layer is included in mask.
     */
    static bool LayerInMask(int layer, int layerMask);

private:
    KL_BEGIN_FIELDS(PhysicsRaycast)
        /* No reflected fields - static utility class. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(PhysicsRaycast)
        KL_SMETHOD_AUTO(PhysicsRaycast::Raycast, "Raycast"),
        KL_SMETHOD_AUTO(PhysicsRaycast::RaycastAll, "Raycast all"),
        KL_SMETHOD_AUTO(PhysicsRaycast::RaycastAny, "Raycast any"),
        KL_SMETHOD_AUTO(PhysicsRaycast::LayerInMask, "Layer in mask")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(PhysicsRaycast)
        /* No reflected ctors - static utility class. */
    KL_END_DESCRIBE(PhysicsRaycast)
};

} // namespace koilo
