// SPDX-License-Identifier: GPL-3.0-or-later
#include "atmosphere.hpp"

#include <algorithm>
#include <cmath>

namespace koilo::aero::isa {

static inline float ClampAltitude(float h) {
    if (h < 0.0f) return 0.0f;
    if (h > kHMax) return kHMax;
    return h;
}

float Temperature(float altitudeM) {
    const float h = ClampAltitude(altitudeM);
    if (h <= kHTrop) {
        return kT0 - kLapse * h;
    }
    return kTTrop;
}

float Pressure(float altitudeM) {
    const float h = ClampAltitude(altitudeM);
    if (h <= kHTrop) {
        // Troposphere: P = P0 * (T/T0)^(g0 / (R*L))
        const float T  = kT0 - kLapse * h;
        const float exponent = kG0 / (kRspec * kLapse);
        return kP0 * std::pow(T / kT0, exponent);
    }
    // Stratosphere (isothermal): P = P_trop * exp(-g0*(h - hTrop) / (R*T_trop))
    const float pTrop = kP0 * std::pow(kTTrop / kT0, kG0 / (kRspec * kLapse));
    return pTrop * std::exp(-kG0 * (h - kHTrop) / (kRspec * kTTrop));
}

float Density(float altitudeM) {
    const float T = Temperature(altitudeM);
    const float P = Pressure(altitudeM);
    return P / (kRspec * T);
}

float SpeedOfSound(float altitudeM) {
    const float T = Temperature(altitudeM);
    return std::sqrt(kGamma * kRspec * T);
}

} // namespace koilo::aero::isa
