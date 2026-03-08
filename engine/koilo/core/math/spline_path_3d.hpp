// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file spline_path_3d.hpp
 * @brief Catmull-Rom spline path for smooth 3D interpolation through control points.
 *
 * @date 07/03/2026
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "vector3d.hpp"
#include <koilo/registry/reflect_macros.hpp>
#include <vector>

namespace koilo {

/**
 * @class SplinePath3D
 * @brief Catmull-Rom spline that passes through all control points in 3D space.
 *
 * Parameter convention: t ranges from 0 to pointCount-1 (or pointCount if looping).
 * Integer values of t correspond exactly to control points. Fractional values
 * interpolate smoothly between adjacent points.
 */
class SplinePath3D {
public:
    SplinePath3D() = default;
    SplinePath3D(const SplinePath3D& other) = default;

    void AddPoint(float x, float y, float z);
    void Clear();
    int GetPointCount() const;
    void SetLooping(bool loop);
    bool GetLooping() const;
    Vector3D GetPoint(int index) const;

    /**
     * @brief Evaluate position on the spline at parameter t.
     * @param t Parameter in range [0, pointCount-1] (clamped or wrapped if looping).
     * @return Interpolated 3D position.
     */
    Vector3D Evaluate(float t) const;

    /**
     * @brief Evaluate the tangent (derivative) at parameter t.
     * @param t Parameter value.
     * @return Tangent vector (not normalized).
     */
    Vector3D EvaluateTangent(float t) const;

private:
    std::vector<Vector3D> points_;
    bool looping_ = false;

    Vector3D GetWrappedPoint(int idx, int n) const;
    static Vector3D CatmullRom(const Vector3D& p0, const Vector3D& p1,
                                const Vector3D& p2, const Vector3D& p3, float t);
    static Vector3D CatmullRomDerivative(const Vector3D& p0, const Vector3D& p1,
                                          const Vector3D& p2, const Vector3D& p3, float t);

public:
    KL_BEGIN_FIELDS(SplinePath3D)
    KL_END_FIELDS

    KL_BEGIN_METHODS(SplinePath3D)
        KL_METHOD_AUTO(SplinePath3D, AddPoint, "Add control point"),
        KL_METHOD_AUTO(SplinePath3D, Clear, "Remove all points"),
        KL_METHOD_AUTO(SplinePath3D, GetPointCount, "Get number of points"),
        KL_METHOD_AUTO(SplinePath3D, SetLooping, "Set looping"),
        KL_METHOD_AUTO(SplinePath3D, GetLooping, "Get looping"),
        KL_METHOD_AUTO(SplinePath3D, GetPoint, "Get point at index"),
        KL_METHOD_AUTO(SplinePath3D, Evaluate, "Evaluate position at t"),
        KL_METHOD_AUTO(SplinePath3D, EvaluateTangent, "Evaluate tangent at t")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SplinePath3D)
        KL_CTOR0(SplinePath3D),
        KL_CTOR(SplinePath3D, const SplinePath3D &)
    KL_END_DESCRIBE(SplinePath3D)
};

} // namespace koilo
