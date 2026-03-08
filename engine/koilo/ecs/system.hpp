// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file system.hpp
 * @brief Base system class for ECS that operates on entities with specific components.
 *
 * TODO: Implement ECS system with the following features:
 * - System registration and execution order
 * - Entity queries (iterate entities with specific components)
 * - System enable/disable
 * - System dependencies and execution ordering
 * - Parallel system execution where possible
 * - System update phases (pre-update, update, post-update, render)
 *
 * Common system examples:
 * - PhysicsSystem (updates transforms based on physics)
 * - RenderSystem (renders meshes)
 * - AnimationSystem (updates animation state)
 * - InputSystem (processes input)
 *
 * @date TBD
 * @author Coela
 */

#pragma once

#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class System
 * @brief Base class for all ECS systems.
 */
class System {
protected:
    bool enabled = true;
    int priority = 0;

public:
    System() = default;
    virtual ~System() = default;

    virtual void Update() = 0;
    
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }
    
    void SetPriority(int priority) { this->priority = priority; }
    int GetPriority() const { return priority; }

    KL_BEGIN_FIELDS(System)
        KL_FIELD(System, enabled, "Enabled", 0, 1),
        KL_FIELD(System, priority, "Priority", -2147483648, 2147483647)
    KL_END_FIELDS

    KL_BEGIN_METHODS(System)
        KL_METHOD_AUTO(System, SetEnabled, "Set enabled"),
        KL_METHOD_AUTO(System, IsEnabled, "Is enabled"),
        KL_METHOD_AUTO(System, SetPriority, "Set priority"),
        KL_METHOD_AUTO(System, GetPriority, "Get priority")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(System)
        /* No reflected ctors - abstract class. */
    KL_END_DESCRIBE(System)
};

} // namespace koilo
