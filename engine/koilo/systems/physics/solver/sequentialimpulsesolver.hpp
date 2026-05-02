// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file sequentialimpulsesolver.hpp
 * @brief sequential-impulse rigid-body constraint solver.
 *
 * Consumes a vector of `ContactManifold`s (produced by the narrowphase) plus
 * a body table indexed by `ColliderProxy::bodyId`, and resolves contact
 * constraints by accumulating normal and tangent impulses with cumulative
 * clamping (Baumgarte stabilisation embedded in the velocity bias).
 *
 * Warm-starting reuses `Contact::accumulatedNormalImpulse` from the previous
 * frame (delivered via `ContactCache::Touch()`). Tangent impulses are stored
 * in the cache for diagnostics but not currently warm-started, since the
 * tangent basis is reconstructed each frame from the contact normal alone
 * and is therefore not stable across frames.
 */

#pragma once

#include "../contactmanifold.hpp"
#include "../contactcache.hpp"

#include <vector>

namespace koilo {

class RigidBody;
class Joint;

/**
 * @struct SolverConfig
 * @brief Tunable parameters for the sequential-impulse solver.
 */
struct SolverConfig {
    int   velocityIterations = 8;
    int   positionIterations = 1;     ///< NGS-style position-correction iterations.
                                      ///< When >0, contact penetration Baumgarte is moved out of the
                                      ///< velocity bias into a separate position pass that mutates a
                                      ///< scratch pose (no KE injection - real linear/angular velocity
                                      ///< untouched). Each iteration re-derives world contact points,
                                      ///< rA/rB, depth, and effective mass from the current scratch
                                      ///< poses (true NGS, not Catto split-impulse - re-linearization
                                      ///< is what makes it converge for stacks). Default 1 is the
                                      ///< sweet spot from the bench harness - strictly better than 0
                                      ///< across all scenes (stack-2/4/8/16 + mass ratios) with no
                                      ///< regressions; higher iteration counts can over-correct and
                                      ///< destabilise mass-ratio scenes. Set to 0 to retain the
                                      ///< pre-6.5b behavior (Baumgarte folded into velocity bias).
    float baumgarteBeta      = 0.2f;  ///< Position-error feedback gain (0..1). Used by velocity-bias
                                      ///< Baumgarte (when positionIterations==0) AND by the NGS
                                      ///< position pass (when positionIterations>0).
    float positionSlop       = 0.005f;///< Allowed penetration before correction kicks in (m).
    float restitutionSlop    = 1.0f;  ///< |vn| below this is treated as a resting contact (no bounce).
    float warmStartScale     = 1.0f;  ///< Multiplier on cached normal impulse at warm-start time.
    float maxLinearCorrection = 0.2f; ///< Cap on per-step positional correction velocity (m). Used by both
                                      ///< Baumgarte (penetrating) and speculative (gap) bias paths to
                                      ///< prevent extreme bias values from causing solver instability.
    float jointBaumgarteBeta = 0.1f;  ///< Baumgarte coefficient for joint position-error correction.
                                      ///< Tuned independently of contact baumgarteBeta - joints typically
                                      ///< benefit from a softer beta to avoid explosive over-correction
                                      ///< in long chains.
};

/**
 * @class SequentialImpulseSolver
 * @brief Stateless solver - call `Solve()` once per substep with the current
 * manifold list, body table, dt, and config.
 *
 * The solver only mutates body linear/angular velocity and writes accumulated
 * impulses back into both the manifold contacts and the contact cache. It
 * does NOT integrate positions; that remains the caller's responsibility
 * (PhysicsWorld integrates after Solve() returns).
 */
class SequentialImpulseSolver {
public:
    /**
     * @brief Resolve all contacts in `manifolds`.
     * @param manifolds  Mutable manifold list. Per-contact accumulated
     *                   impulses are updated in place.
     * @param bodies     Body table indexed by `ColliderProxy::bodyId`. Entries
     *                   may be null (treated as static / immovable).
     * @param cache      Contact cache; receives writeback of accumulated
     *                   impulses for the next frame's warm-start.
     * @param dt         Substep duration in seconds (>0).
     * @param cfg        Solver tuning parameters.
     */
    void Solve(std::vector<ContactManifold>& manifolds,
               const std::vector<RigidBody*>& bodies,
               ContactCache& cache,
               float dt,
               const SolverConfig& cfg);

    /**
     * @brief overload: solve contacts AND joints together. Runs even
     * when `manifolds` is empty (joint-only scenes are valid).
     *
     * Joints are solved alongside contacts in the same velocity-iteration
     * pass - this is the standard Box2D/PhysX-style sequential-impulse
     * scheme, where each iteration sweeps all rows once. Joints store their
     * own per-row accumulated impulses for next-frame warm-start (no
     * external cache needed; joint identity is stable).
     */
    void Solve(std::vector<ContactManifold>& manifolds,
               const std::vector<RigidBody*>& bodies,
               const std::vector<Joint*>& joints,
               ContactCache& cache,
               float dt,
               const SolverConfig& cfg);
};

} // namespace koilo
