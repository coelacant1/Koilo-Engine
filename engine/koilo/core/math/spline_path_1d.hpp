// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file spline_path_1d.hpp
 * @brief Catmull-Rom spline for scalar values (angles, intensity, etc).
 *
 * @date 07/03/2026
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <koilo/registry/reflect_macros.hpp>
#include <vector>

namespace koilo {

/**
 * @class SplinePath1D
 * @brief Catmull-Rom spline for scalar values with optional angle wrapping.
 *
 * Same parameter convention as SplinePath3D. Optionally handles angle wrapping
 * for yaw/pitch interpolation.
 */
class SplinePath1D {
public:
    SplinePath1D() = default;
    SplinePath1D(const SplinePath1D& other) = default;

    void AddPoint(float value);
    void Clear();
    int GetPointCount() const;
    void SetLooping(bool loop);
    bool GetLooping() const;

    /**
     * @brief Enable angle wrapping mode for angular values (e.g. yaw in degrees).
     *
     * When enabled, values are unwrapped to be continuous before interpolation,
     * then the result is wrapped back to [0, 360).
     */
    void SetAngleMode(bool angle);

    float GetPoint(int index) const;
    float Evaluate(float t) const;

private:
    std::vector<float> values_;
    std::vector<float> unwrapped_;
    bool looping_ = false;
    bool angleMode_ = false;

    void Unwrap();
    float WrapResult(float v) const;
    float GetWrapped(int idx, const std::vector<float>& pts) const;
    static float CatmullRom1D(float p0, float p1, float p2, float p3, float t);

public:
    KL_BEGIN_FIELDS(SplinePath1D)
    KL_END_FIELDS

    KL_BEGIN_METHODS(SplinePath1D)
        KL_METHOD_AUTO(SplinePath1D, AddPoint, "Add scalar value"),
        KL_METHOD_AUTO(SplinePath1D, Clear, "Remove all points"),
        KL_METHOD_AUTO(SplinePath1D, GetPointCount, "Get number of points"),
        KL_METHOD_AUTO(SplinePath1D, SetLooping, "Set looping"),
        KL_METHOD_AUTO(SplinePath1D, GetLooping, "Get looping"),
        KL_METHOD_AUTO(SplinePath1D, SetAngleMode, "Enable angle wrapping"),
        KL_METHOD_AUTO(SplinePath1D, GetPoint, "Get value at index"),
        KL_METHOD_AUTO(SplinePath1D, Evaluate, "Evaluate at t")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SplinePath1D)
        KL_CTOR0(SplinePath1D),
        KL_CTOR(SplinePath1D, const SplinePath1D &)
    KL_END_DESCRIBE(SplinePath1D)
};

} // namespace koilo
