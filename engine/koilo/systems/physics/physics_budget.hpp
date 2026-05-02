// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file physics_budget.hpp
 * @brief Real-time physics step budget + policy/determinism enums.
 */

#pragma once

#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct PhysicsBudget
 * @brief Bounds the cost of a single PhysicsWorld::Step call.
 *
 * Step bails (drops remaining substeps) when either limit is exceeded and
 * records an overrun in the world. Default keeps the spiral-of-death tame
 * for real-time loops.
 */
struct PhysicsBudget {
    float maxStepMs;   ///< Wall-clock cap for one Step() call. <=0 disables.
    int   maxSubsteps; ///< Hard cap on substeps per Step() call.

    PhysicsBudget() : maxStepMs(4.0f), maxSubsteps(4) {}
    PhysicsBudget(float ms, int steps) : maxStepMs(ms), maxSubsteps(steps) {}

    KL_BEGIN_FIELDS(PhysicsBudget)
        KL_FIELD(PhysicsBudget, maxStepMs, "Max step ms", 0, 0),
        KL_FIELD(PhysicsBudget, maxSubsteps, "Max substeps", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(PhysicsBudget)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(PhysicsBudget)
        KL_CTOR0(PhysicsBudget)
    KL_END_DESCRIBE(PhysicsBudget)
};

/**
 * @enum PhysicsTransformPolicy
 * @brief Direction of pose sync between physics and ECS / scene.
 *
 * The default ships with PhysicsAuthoritative; tune as broadphase profiling
 * data becomes available.
 */
enum class PhysicsTransformPolicy {
    PhysicsAuthoritative, ///< Physics owns pose; ECS reads at end-of-step.
    EcsAuthoritative,     ///< ECS owns pose; physics imports each step.
    Bidirectional         ///< Both sides may write; physics resolves.
};

/**
 * @enum DeterminismTier
 * @brief Determinism guarantees per world.
 *
 *  - T0 best-effort (FX, eye candy)
 *  - T1 same-binary replay (recommended for gameplay; default)
 *  - T2 bit-exact cross-machine (opt-in; ships only when needed)
 */
enum class DeterminismTier {
    T0_BestEffort,
    T1_SameBinaryReplay,
    T2_BitExactCrossMachine
};

} // namespace koilo
