// SPDX-License-Identifier: GPL-3.0-or-later
#include "aerocurve.hpp"

#include <algorithm>
#include <cassert>

namespace koilo::aero {

AeroCurve::AeroCurve(std::vector<float> aoaRad, std::vector<float> values) {
    Set(std::move(aoaRad), std::move(values));
}

void AeroCurve::Set(std::vector<float> aoaRad, std::vector<float> values) {
    assert(aoaRad.size() == values.size());
    aoa_   = std::move(aoaRad);
    value_ = std::move(values);
}

float AeroCurve::Sample(float alphaRad) const {
    if (aoa_.empty()) return 0.0f;
    if (aoa_.size() == 1) return value_[0];
    if (alphaRad <= aoa_.front()) return value_.front();
    if (alphaRad >= aoa_.back())  return value_.back();
    // Binary search for upper bound.
    auto it = std::upper_bound(aoa_.begin(), aoa_.end(), alphaRad);
    const std::size_t hi = static_cast<std::size_t>(it - aoa_.begin());
    const std::size_t lo = hi - 1;
    const float aLo = aoa_[lo];
    const float aHi = aoa_[hi];
    const float span = aHi - aLo;
    if (span <= 0.0f) return value_[lo];
    const float t = (alphaRad - aLo) / span;
    return value_[lo] + (value_[hi] - value_[lo]) * t;
}

AeroCurve AeroCurve::MakeFlatPlateLift() {
    // alpha [rad]: -0.5, -0.26, 0, 0.26, 0.5
    // CL: stalls past +/- 0.26 rad.
    return AeroCurve(
        {-0.5f, -0.26f, 0.0f, 0.26f, 0.5f},
        {-0.5f, -1.2f,  0.0f, 1.2f,  0.5f}
    );
}

AeroCurve AeroCurve::MakeFlatPlateDrag() {
    // CD = 0.02 + 0.5 * alpha^2 sampled.
    return AeroCurve(
        {-0.5f, -0.26f, 0.0f, 0.26f, 0.5f},
        {0.145f, 0.054f, 0.02f, 0.054f, 0.145f}
    );
}

void AeroCurve::InitFlatPlateLift() {
    *this = MakeFlatPlateLift();
}

void AeroCurve::InitFlatPlateDrag() {
    *this = MakeFlatPlateDrag();
}

} // namespace koilo::aero
