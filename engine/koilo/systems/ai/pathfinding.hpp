// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file pathfinding.hpp
 * @brief Agent configuration for pathfinding.
 *
 * Actual pathfinding algorithms are in pathfinder.hpp (PathfinderGrid with A*).
 * This class holds per-agent navigation parameters.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class Pathfinding
 * @brief Agent navigation configuration.
 */
class Pathfinding {
protected:
    float agentRadius = 0.5f;
    float agentHeight = 2.0f;
    bool initialized = false;

public:
    Pathfinding() = default;

    void Initialize() { initialized = true; }
    bool IsInitialized() const { return initialized; }
    
    void SetAgentRadius(float radius) { agentRadius = radius; }
    float GetAgentRadius() const { return agentRadius; }
    
    void SetAgentHeight(float height) { agentHeight = height; }
    float GetAgentHeight() const { return agentHeight; }

    KL_BEGIN_FIELDS(Pathfinding)
        KL_FIELD(Pathfinding, agentRadius, "Agent radius", 0.0f, 10.0f),
        KL_FIELD(Pathfinding, agentHeight, "Agent height", 0.0f, 10.0f),
        KL_FIELD(Pathfinding, initialized, "Initialized", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Pathfinding)
        KL_METHOD_AUTO(Pathfinding, Initialize, "Initialize"),
        KL_METHOD_AUTO(Pathfinding, IsInitialized, "Is initialized"),
        KL_METHOD_AUTO(Pathfinding, SetAgentRadius, "Set agent radius"),
        KL_METHOD_AUTO(Pathfinding, GetAgentRadius, "Get agent radius"),
        KL_METHOD_AUTO(Pathfinding, SetAgentHeight, "Set agent height"),
        KL_METHOD_AUTO(Pathfinding, GetAgentHeight, "Get agent height")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Pathfinding)
        KL_CTOR0(Pathfinding)
    KL_END_DESCRIBE(Pathfinding)
};

} // namespace koilo
