// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file atmosphere.hpp
 * @brief International Standard Atmosphere (ISA) model.
 *
 * Free functions returning density / pressure / temperature / speed of sound
 * as a function of geometric altitude in metres above mean sea level. Two
 * layers are modelled:
 *   - Troposphere      [0 m, 11 000 m): linear lapse, L = 6.5 K/km
 *   - Lower stratosphere [11 000 m, 20 000 m]: isothermal at 216.65 K
 *
 * Sea-level reference values (ISA): T0 = 288.15 K, P0 = 101 325 Pa,
 * rho0 = 1.225 kg/m^3, R_specific = 287.05 J/(kg*K), gamma = 1.4,
 * g0 = 9.80665 m/s^2.
 *
 * Determinism: T0/T1. NOT bit-exact across machines under T2 because the
 * pressure formulas use std::pow / std::exp, which are libc-defined and
 * not IEEE-correctly-rounded across libcs / architectures.
 * ships T1-only; T2 builds will still link this code but cross-machine
 * bit-exactness is NOT promised.
 *
 * Coordinate convention: callers pass scalar altitude in metres. The world
 * frame in Koilo uses +Y as altitude by default - see AerodynamicsWorld
 * for how altitude is extracted from a body pose.
 */

#pragma once

namespace koilo::aero::isa {

/// Sea-level temperature [K].
constexpr float kT0     = 288.15f;
/// Sea-level pressure [Pa].
constexpr float kP0     = 101325.0f;
/// Sea-level density [kg/m^3].
constexpr float kRho0   = 1.225f;
/// Specific gas constant for dry air [J/(kg*K)].
constexpr float kRspec  = 287.05f;
/// Adiabatic index of dry air.
constexpr float kGamma  = 1.4f;
/// Standard gravity [m/s^2].
constexpr float kG0     = 9.80665f;
/// Tropospheric lapse rate [K/m] (positive = magnitude; T decreases with altitude).
constexpr float kLapse  = 0.0065f;
/// Tropopause altitude [m].
constexpr float kHTrop  = 11000.0f;
/// Temperature at tropopause [K].
constexpr float kTTrop  = kT0 - kLapse * kHTrop; // 216.65 K
/// Top of supported atmosphere (lower stratosphere) [m].
constexpr float kHMax   = 20000.0f;

/**
 * @brief Returns ISA temperature at the given geometric altitude [m].
 * Clamps altitude to [0, kHMax].
 */
float Temperature(float altitudeM);

/**
 * @brief Returns ISA static pressure at the given geometric altitude [m].
 * Clamps altitude to [0, kHMax]. Uses std::pow in the troposphere and
 * std::exp in the stratosphere.
 */
float Pressure(float altitudeM);

/**
 * @brief Returns ISA density at the given geometric altitude [m].
 * Computed from rho = P / (R_specific * T).
 */
float Density(float altitudeM);

/**
 * @brief Returns ISA speed of sound at the given geometric altitude [m].
 * Computed from a = sqrt(gamma * R_specific * T).
 */
float SpeedOfSound(float altitudeM);

} // namespace koilo::aero::isa
