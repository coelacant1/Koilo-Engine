// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file thrustengine.hpp
 * @brief Body-fixed thrust engine with optional fuel burn.
 *
 * A ThrustEngine produces a force F = throttle * maxThrustN along
 * directionLocal (rotated into world by the parent body's orientation),
 * applied at applicationPointLocal (rotated + translated). When fuel is
 * tracked (specificImpulseSec > 0 and fuelMassKg > 0) the engine burns
 * fuel at mass-flow mdot = thrustN / (specificImpulseSec * g0), where
 * g0 = 9.80665 m/s^2 (standard gravity, NOT local gravity - this is the
 * conventional definition of Isp).
 *
 * On fuel burn the parent body's mass is updated via SetMass(dryMassKg
 * + fuelMassKg). Inertia is NOT recomputed from the new mass - see
 * plan for the documented limitation. Asymmetric tank emptying
 * (CoM drift) is also not modelled.
 *
 * directionLocal is normalized at compute time so authored values may be
 * non-unit (a user-friendly default).
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo::aero {

struct ThrustEngine {
    /// Engine nozzle direction in body-local frame; the resultant thrust
    /// vector points along this axis (rotated to world by body orientation).
    Vector3D directionLocal{1.0f, 0.0f, 0.0f};
    /// Application point in body-local frame.
    Vector3D applicationPointLocal{0.0f, 0.0f, 0.0f};
    /// Throttle in [0, 1]. Clamped at compute time.
    float    throttle    = 0.0f;
    /// Max thrust at full throttle [N].
    float    maxThrustN  = 0.0f;
    /// Specific impulse [s]. Set <= 0 to disable fuel burn.
    float    specificImpulseSec = 0.0f;
    /// Current fuel mass [kg].
    float    fuelMassKg  = 0.0f;
    /// Dry mass of the body (without fuel) [kg]. Required for fuel burn:
    /// body mass = dryMassKg + fuelMassKg is rewritten each substep.
    float    dryMassKg   = 0.0f;

    KL_BEGIN_FIELDS(ThrustEngine)
        KL_FIELD(ThrustEngine, directionLocal, "Direction (local)", 0, 0),
        KL_FIELD(ThrustEngine, applicationPointLocal, "Application point (local)", 0, 0),
        KL_FIELD(ThrustEngine, throttle, "Throttle [0,1]", 0, 1),
        KL_FIELD(ThrustEngine, maxThrustN, "Max thrust (N)", 0, 0),
        KL_FIELD(ThrustEngine, specificImpulseSec, "Specific impulse (s)", 0, 0),
        KL_FIELD(ThrustEngine, fuelMassKg, "Fuel mass (kg)", 0, 0),
        KL_FIELD(ThrustEngine, dryMassKg, "Dry mass (kg)", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ThrustEngine)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ThrustEngine)
        KL_CTOR0(ThrustEngine)
    KL_END_DESCRIBE(ThrustEngine)
};

} // namespace koilo::aero
