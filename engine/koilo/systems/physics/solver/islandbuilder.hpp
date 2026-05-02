// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file islandbuilder.hpp
 * @brief union-find over the contact graph.
 *
 * Groups bodies into islands based on which manifolds reference them.
 * Static/sleeping bodies do NOT propagate islands - a sleeping body that
 * becomes a "bridge" between two awake bodies should be re-evaluated; this
 * is the responsibility of the SleepManager which calls IslandBuilder twice
 * if needed (once for wake propagation, once for the sleep decision).
 *
 * Determinism: union-by-lowest-id. Each island's canonical id is the
 * minimum body id in the island. Output islands are sorted by canonical id.
 *
 * Reuse: islands can also be used to drive parallel solver dispatch in a
 * future phase (independent islands have no shared state).
 */

#pragma once

#include "../contactmanifold.hpp"

#include <cstdint>
#include <vector>

namespace koilo {

class RigidBody;
class Joint;

/**
 * @struct Island
 * @brief A connected component of the body/contact graph.
 */
struct Island {
    std::uint32_t canonicalId = 0;          ///< Minimum body id in the island (deterministic).
    std::vector<std::uint32_t> bodyIds;     ///< Member dynamic body ids (sorted).
    std::vector<std::uint32_t> manifoldIdx; ///< Indices into the manifold list.
};

/**
 * @class IslandBuilder
 * @brief Pure function-style helper. No persistent state.
 */
class IslandBuilder {
public:
    /**
     * @brief Builds islands from `manifolds` over the body table.
     *
     * Edges: for each manifold, an edge between its two bodies (skipped if
     * either side is null/static/sleeping; treat-as-anchor - does not
     * propagate connectivity).
     *
     * @param manifolds Contact manifolds.
     * @param bodies    Body table indexed by ColliderProxy::bodyId.
     * @param treatSleepingAsBridge If true, sleeping bodies still propagate
     *        connectivity (used by the wake pass so a moving body can wake an
     *        entire chain). If false, sleeping bodies are anchors (the normal
     *        sleep decision pass).
     * @return Island list, sorted by canonicalId ascending.
     */
    static std::vector<Island> Build(const std::vector<ContactManifold>& manifolds,
                                     const std::vector<RigidBody*>& bodies,
                                     bool treatSleepingAsBridge);

    /**
     * @brief overload: also union bodies sharing a joint. Joint
     * endpoints propagate connectivity exactly like contact pairs, so a
     * pendulum's bob can wake or be slept-with its anchor body.
     */
    static std::vector<Island> Build(const std::vector<ContactManifold>& manifolds,
                                     const std::vector<Joint*>& joints,
                                     const std::vector<RigidBody*>& bodies,
                                     bool treatSleepingAsBridge);
};

} // namespace koilo
