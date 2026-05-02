// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file sleepmanager.hpp
 * @brief per-island sleep/wake decisions.
 *
 * Workflow per substep (called by PhysicsWorld in 6c):
 *   1. SleepManager::WakeIslandsWithMotion(...)  // before solver
 *      Forces wake any sleeping body that shares an island with a body
 *      whose kinetic energy exceeds the wake threshold.
 *   2. solver.Solve(...)
 *   3. integrate positions
 *   4. SleepManager::AttemptSleep(...)            // after integration
 *      Per island: if every body's kinetic energy stays below
 *      `sleepEnergyThreshold` for `sleepTimeRequired` continuous seconds,
 *      put the entire island to sleep.
 */

#pragma once

#include "islandbuilder.hpp"

#include <cstdint>
#include <vector>

namespace koilo {

class RigidBody;

struct SleepConfig {
    float sleepEnergyThreshold = 0.05f;  ///< J - total KE per body to count as "quiet".
    float sleepTimeRequired    = 0.5f;   ///< seconds of continuous quiet before sleep.
    float wakeEnergyThreshold  = 0.10f;  ///< J - wake island if any body exceeds this.
};

class SleepManager {
public:
    /**
     * @brief Wakes any sleeping body that shares an island with an awake-and-moving
     * body (KE > wakeEnergyThreshold). Call BEFORE the solver runs.
     *
     * Pass `bridgeIslands` built with `treatSleepingAsBridge=true` so the chain
     * propagates through still-sleeping intermediate bodies.
     */
    static void WakeIslandsWithMotion(const std::vector<Island>& bridgeIslands,
                                      const std::vector<RigidBody*>& bodies,
                                      const SleepConfig& cfg);

    /**
     * @brief Per island, accumulates per-body sleep timers and puts the entire
     * island to sleep when all members have been quiet long enough.
     *
     * Pass `islands` built with `treatSleepingAsBridge=false` so newly sleeping
     * bodies anchor (don't accidentally bind together separate awake systems).
     *
     * Skip an island entirely if any member has `GetAllowSleep() == false`.
     */
    static void AttemptSleep(const std::vector<Island>& islands,
                             const std::vector<RigidBody*>& bodies,
                             float dt,
                             const SleepConfig& cfg);

    /** Computes (1/2)·m·v² + (1/2)·ω·I·ω for one body. */
    static float KineticEnergy(const RigidBody& rb);
};

} // namespace koilo
