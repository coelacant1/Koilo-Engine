// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file aerodynamicsurface.hpp
 * @brief Single planar aerodynamic surface (wing / control surface).
 *
 * Storage-only component: the AerodynamicsWorld owns the per-substep
 * compute. A surface is described in its parent body's local frame by
 *   - centerOfPressureLocal: the application point of the resultant force.
 *   - forwardLocal:          chord-aligned axis (nose direction of the chord).
 *   - upLocal:               chord-normal axis (positive lift direction at zero AoA).
 * forwardLocal and upLocal are orthonormalized lazily at compute time so
 * authored values do not need to be perfectly orthogonal.
 *
 * Limitations (v0):
 *   - Sideslip is treated as ignored: only the chord-plane projection of
 *     the relative airflow contributes to dynamic pressure and AoA.
 *   - controlDeflectionRad is added directly to AoA (simple flap model;
 *     no explicit chord rotation, no hinge moment).
 * - No pitching moment coefficient (cm) - returns no torque
 *     beyond the lever-arm torque from applying force at the COP.
 *   - Reverse-flow regime ([-pi, pi] AoA wrap) is NOT modelled: AoA wraps
 *     to atan2 range but lift/drag curves are clamped at their endpoints.
 */

#pragma once

#include "aerocurve.hpp"
#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo::aero {

struct AerodynamicSurface {
    Vector3D centerOfPressureLocal{0.0f, 0.0f, 0.0f};
    Vector3D forwardLocal{1.0f, 0.0f, 0.0f};
    Vector3D upLocal{0.0f, 1.0f, 0.0f};
    /// Wing planform area [m^2].
    float    area = 1.0f;
    /// Control surface deflection [rad], added to AoA before lift/drag lookup.
    float    controlDeflectionRad = 0.0f;
    AeroCurve cl;
    AeroCurve cd;

    // --- Read-only debug data populated by AerodynamicsWorld::ApplySurface.
    // Updated each substep so scripts/HUD can visualize per-fin force.
    Vector3D lastForceWorld{0.0f, 0.0f, 0.0f};
    Vector3D lastCopWorld{0.0f, 0.0f, 0.0f};
    float    lastAoaRad   = 0.0f;
    float    lastDynPress = 0.0f;

    KL_BEGIN_FIELDS(AerodynamicSurface)
        KL_FIELD(AerodynamicSurface, centerOfPressureLocal, "Center of pressure (local)", 0, 0),
        KL_FIELD(AerodynamicSurface, forwardLocal, "Forward (local)", 0, 0),
        KL_FIELD(AerodynamicSurface, upLocal, "Up (local)", 0, 0),
        KL_FIELD(AerodynamicSurface, area, "Area (m^2)", 0, 0),
        KL_FIELD(AerodynamicSurface, controlDeflectionRad, "Control deflection (rad)", 0, 0),
        KL_FIELD(AerodynamicSurface, cl, "CL curve", 0, 0),
        KL_FIELD(AerodynamicSurface, cd, "CD curve", 0, 0),
        KL_FIELD(AerodynamicSurface, lastForceWorld, "Last force (world)", 0, 0),
        KL_FIELD(AerodynamicSurface, lastCopWorld, "Last COP (world)", 0, 0),
        KL_FIELD(AerodynamicSurface, lastAoaRad, "Last AoA (rad)", 0, 0),
        KL_FIELD(AerodynamicSurface, lastDynPress, "Last dynamic pressure", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(AerodynamicSurface)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AerodynamicSurface)
        KL_CTOR0(AerodynamicSurface)
    KL_END_DESCRIBE(AerodynamicSurface)
};

} // namespace koilo::aero
