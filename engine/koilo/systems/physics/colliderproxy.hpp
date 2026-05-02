// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file colliderproxy.hpp
 * @brief Broadphase/narrowphase view of a collider.
 *
 * What the broadphase indexes and the narrowphase consumes. Existing
 * Collider subclasses are wired into proxies, which downstream stages
 * consume directly.
 */

#pragma once

#include <koilo/systems/physics/bodypose.hpp>
#include <koilo/systems/physics/shape/ishape.hpp>
#include <koilo/core/geometry/3d/aabb.hpp>
#include <cstdint>

namespace koilo {

/**
 * @class ColliderProxy
 * @brief Lightweight narrowphase-facing record. Owns no shape memory.
 *
 * `shape` is a non-owning pointer; the owning collider/component is
 * responsible for its lifetime. `worldAabb` is rebuilt by the broadphase
 * each step from `shape->WorldAABB(bodyPose * localOffset)`.
 */
class ColliderProxy {
public:
    static constexpr std::uint32_t kInvalidHandle = static_cast<std::uint32_t>(-1);

    IShape*       shape          = nullptr;
    BodyPose      localOffset;                ///< Offset in the parent body's local frame.
    std::uint32_t layer          = 0;         ///< Collision layer (0..31).
    std::uint32_t broadphaseHandle = kInvalidHandle;
    std::uint32_t bodyId         = kInvalidHandle; ///< Index into rigid-body table.
    std::uint32_t proxyId        = kInvalidHandle; ///< Stable per-proxy id. Used for deterministic pair ordering and contact-cache keys.
    AABB          worldAabb;                  ///< Refreshed each broadphase pass.
    bool          isTrigger      = false;
    bool          enabled        = true;

    /** Recomputes worldAabb from the supplied body pose. */
    void RefreshWorldAABB(const BodyPose& bodyPose) {
        if (!shape) {
            worldAabb = AABB(bodyPose.position, bodyPose.position);
            return;
        }
        worldAabb = shape->WorldAABB(bodyPose.Compose(localOffset));
    }
};

} // namespace koilo
