// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file aerocurve.hpp
 * @brief Piecewise-linear lookup table keyed on angle of attack (radians).
 *
 * Used for CL(alpha), CD(alpha) curves on aerodynamic surfaces. Storage is
 * two parallel sorted vectors so a binary search + lerp is O(log N) per
 * lookup with no heap allocation per call. Endpoints are clamped (no
 * extrapolation).
 *
 * For bit-exactness within a single binary, the lookup is reproducible:
 * given the same input alpha and same table contents the output bytes are
 * identical. (No transcendentals; only multiply/add.)
 */

#pragma once

#include <cstddef>
#include <vector>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo::aero {

class AeroCurve {
public:
    AeroCurve() = default;

    /**
     * @brief Construct from sorted parallel arrays. Asserts (in debug)
     * that aoaRad is strictly increasing and the two vectors have equal
     * size. An empty curve always returns 0.
     */
    AeroCurve(std::vector<float> aoaRad, std::vector<float> values);

    /**
     * @brief Reset the table. Inputs MUST be sorted ascending by aoaRad
     * with equal length, else behavior is undefined.
     */
    void Set(std::vector<float> aoaRad, std::vector<float> values);

    /**
     * @brief Sample the curve at angle of attack alpha (radians).
     * Out-of-range alpha clamps to the endpoint values. Empty curves return 0.
     */
    float Sample(float alphaRad) const;

    std::size_t Size() const { return aoa_.size(); }
    bool        Empty() const { return aoa_.empty(); }

    /**
     * @brief Symmetric thin-plate-ish CL approximation: linear with
     * stall around +/- 0.26 rad (15 deg). Suitable for sanity tests, not
     * real flight modelling.
     */
    static AeroCurve MakeFlatPlateLift();

    /**
     * @brief In-place version of MakeFlatPlateLift, populating *this*.
     * Useful from script bindings where static factories aren't reflected.
     */
    void InitFlatPlateLift();

    /**
     * @brief Constant-base + AoA^2 drag curve, CD0 + k * alpha^2.
     */
    static AeroCurve MakeFlatPlateDrag();

    /**
     * @brief In-place version of MakeFlatPlateDrag, populating *this*.
     */
    void InitFlatPlateDrag();

private:
    std::vector<float> aoa_;
    std::vector<float> value_;

public:
    KL_BEGIN_FIELDS(AeroCurve)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AeroCurve)
        KL_METHOD_AUTO(AeroCurve, Sample, "Sample the curve at AoA (rad)"),
        KL_METHOD_AUTO(AeroCurve, Size, "Number of samples"),
        KL_METHOD_AUTO(AeroCurve, Empty, "True if no samples"),
        KL_METHOD_AUTO(AeroCurve, InitFlatPlateLift, "Populate with the symmetric flat-plate lift curve"),
        KL_METHOD_AUTO(AeroCurve, InitFlatPlateDrag, "Populate with the AoA^2 flat-plate drag curve")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AeroCurve)
        KL_CTOR0(AeroCurve)
    KL_END_DESCRIBE_COPYABLE(AeroCurve)
};

} // namespace koilo::aero
