// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file physicsdiagnostics.hpp
 * @brief conservation diagnostics for the physics world.
 *
 * Aggregates total linear momentum, total angular momentum (about the world
 * origin), and total kinetic energy across all dynamic bodies. Computed on
 * demand by `PhysicsWorld::ComputeDiagnostics()` - there is no per-step
 * cost. Static and kinematic bodies are excluded (they are externally
 * driven, so their "momentum" is not meaningful for conservation tests).
 *
 * Sleeping bodies ARE included: they carry zero velocity by definition, so
 * including them is correct and avoids skewing totals when an island sleeps
 * mid-test.
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct PhysicsDiagnostics
 * @brief Snapshot of conservation quantities at a point in time.
 *
 * `linearMomentum   = Σ mᵢ vᵢ`
 * `angularMomentum  = Σ (rᵢ × mᵢ vᵢ + Iᵢ_world ωᵢ)`   about world origin
 * `kineticEnergy    = Σ (½ mᵢ |vᵢ|² + ½ ωᵢ · Iᵢ_world ωᵢ)`
 */
struct PhysicsDiagnostics {
    Vector3D linearMomentum{0, 0, 0};
    Vector3D angularMomentum{0, 0, 0};
    float    kineticEnergy = 0.0f;
    int      bodyCount = 0;          ///< Number of dynamic bodies summed.

    KL_BEGIN_FIELDS(PhysicsDiagnostics)
        KL_FIELD(PhysicsDiagnostics, linearMomentum,  "Linear momentum",  0, 0),
        KL_FIELD(PhysicsDiagnostics, angularMomentum, "Angular momentum", 0, 0),
        KL_FIELD(PhysicsDiagnostics, kineticEnergy,   "Kinetic energy",   0, 0),
        KL_FIELD(PhysicsDiagnostics, bodyCount,       "Body count",       0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(PhysicsDiagnostics)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(PhysicsDiagnostics)
        KL_CTOR0(PhysicsDiagnostics)
    KL_END_DESCRIBE(PhysicsDiagnostics)
};

} // namespace koilo
