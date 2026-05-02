// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file windfield.hpp
 * @brief Wind sampling abstraction for aerodynamics.
 *
 * IWindField returns the local wind velocity vector in WORLD coordinates
 * at a given world-space point and (deterministic) simulation time in
 * seconds. ships two concrete fields:
 *   - ConstantWind: same world-space velocity everywhere, time-independent.
 *   - ShearWind:    base velocity + linear gradient with altitude (world Y).
 *
 * 3D procedural / VectorField3D-backed winds are deferred - the abstract
 * interface here is the future extension point.
 */

#pragma once

#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo::aero {

class IWindField {
public:
    virtual ~IWindField() = default;

    /**
     * @brief Sample wind velocity in world coordinates [m/s].
     * @param worldPos The world-space sample point [m].
     * @param simTimeSec Deterministic simulation time [s] (sum of fixedDt
     *                   substeps since module init). Use it for time-varying
     *                   gusts; constant fields ignore it.
     */
    virtual Vector3D Sample(const Vector3D& worldPos, float simTimeSec) const = 0;
};

/**
 * @class ConstantWind
 * @brief Spatially / temporally constant wind velocity.
 */
class ConstantWind : public IWindField {
public:
    ConstantWind() = default;
    explicit ConstantWind(const Vector3D& velocity) : velocity_(velocity) {}

    void SetVelocity(const Vector3D& v) { velocity_ = v; }
    const Vector3D& GetVelocity() const { return velocity_; }

    Vector3D Sample(const Vector3D& /*worldPos*/, float /*simTimeSec*/) const override {
        return velocity_;
    }

private:
    Vector3D velocity_{0.0f, 0.0f, 0.0f};

public:
    KL_BEGIN_FIELDS(ConstantWind)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(ConstantWind)
        KL_METHOD_AUTO(ConstantWind, SetVelocity, "Set wind velocity (world m/s)"),
        KL_METHOD_AUTO(ConstantWind, GetVelocity, "Get wind velocity (world m/s)"),
        KL_METHOD_AUTO(ConstantWind, Sample, "Sample wind at point/time")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ConstantWind)
        KL_CTOR0(ConstantWind),
        KL_CTOR(ConstantWind, Vector3D)
    KL_END_DESCRIBE(ConstantWind)
};

/**
 * @class ShearWind
 * @brief Wind that varies linearly with altitude (world +Y axis).
 *
 * sample = base + perMetreGradient * worldPos.Y
 */
class ShearWind : public IWindField {
public:
    ShearWind() = default;
    ShearWind(const Vector3D& base, const Vector3D& perMetreGradient)
        : base_(base), gradient_(perMetreGradient) {}

    Vector3D Sample(const Vector3D& worldPos, float /*simTimeSec*/) const override {
        return base_ + gradient_ * worldPos.Y;
    }

    void SetBase(const Vector3D& v)      { base_ = v; }
    void SetGradient(const Vector3D& g)  { gradient_ = g; }

private:
    Vector3D base_{0.0f, 0.0f, 0.0f};
    Vector3D gradient_{0.0f, 0.0f, 0.0f};

public:
    KL_BEGIN_FIELDS(ShearWind)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(ShearWind)
        KL_METHOD_AUTO(ShearWind, SetBase, "Set base wind velocity"),
        KL_METHOD_AUTO(ShearWind, SetGradient, "Set per-metre wind gradient (world Y)"),
        KL_METHOD_AUTO(ShearWind, Sample, "Sample wind at point/time")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ShearWind)
        KL_CTOR0(ShearWind),
        KL_CTOR(ShearWind, Vector3D, Vector3D)
    KL_END_DESCRIBE(ShearWind)
};

} // namespace koilo::aero
