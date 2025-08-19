#pragma once

#include <cmath>
#include <cstdint>

#include "../ishader.hpp"
#include "../../material/materialt.hpp"   // for IMaterial::As<T>()
#include "gradientparams.hpp"

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/math/eulerangles.hpp"
#include "../../../../core/math/eulerconstants.hpp"
#include "../../../../core/math/rotation.hpp"
#include "../../../../core/math/quaternion.hpp"

/**
 * @file gradientshader.hpp
 * @brief Stateless shader that samples a linear or radial N-key gradient.
 *
 * Uses the bound material’s @ref GradientParams to compute a phase along +X (linear)
 * or radial distance (radial), applies optional rotation about a configurable origin,
 * and returns either stepped or interpolated color between adjacent keys.
 *
 * @tparam N Number of colors (keys) in the gradient.
 */

/**
 * @tparam N Number of colors in the gradient.
 * @class GradientShaderT
 * @brief Implements gradient sampling for materials mixing in @ref GradientParams.
 */
template <size_t N>
class GradientShaderT final : public IShader {
public:
    /**
     * @brief Shade a surface point by evaluating the configured gradient.
     * @param surf Surface properties (position used; XY plane evaluation).
     * @param m    Bound material; expected to be @c MaterialT<GradientParams<N>, GradientShaderT<N>>.
     * @return RGB color sampled from the gradient.
     *
     * @details
     * Steps:
     * 1. Optionally rotate @p surf.position about @c rotationOffset by @c rotationAngle (deg).
     * 2. Translate by @c positionOffset and apply @c gradientShift along +X.
     * 3. Compute phase: linear uses X; radial uses sqrt(X²+Y²); both wrapped by @c gradientPeriod.
     * 4. Map phase into [0,N), pick indices i0 and i1=(i0+1)%N.
     * 5. If @c isStepped, return @c colors[i0]; otherwise interpolate i0->i1 by the fractional part.
     */
    RGBColor Shade(const SurfaceProperties& surf, const IMaterial& m) const override {
        using MatBase = MaterialT<GradientParams<N>, GradientShaderT<N>>;
        const auto& P = m.As<MatBase>();

        // --- Position prep (XY plane), with rotation about P.rotationOffset
        Vector3D pos = surf.position;

        if (P.rotationAngle != 0.0f) {
            // translate to origin around rotationOffset, rotate, translate back
            pos = pos - Vector3D(P.rotationOffset.X, P.rotationOffset.Y, 0.0f);

            Quaternion q = Rotation(
                EulerAngles(Vector3D(0.0f, 0.0f, P.rotationAngle),
                            EulerConstants::EulerOrderXYZS)
            ).GetQuaternion();

            pos = q.RotateVector(pos);

            pos = pos + Vector3D(P.rotationOffset.X, P.rotationOffset.Y, 0.0f);
        }

        // shift, then apply gradient phase shift along +X
        pos = pos - Vector3D(P.positionOffset.X, P.positionOffset.Y, 0.0f);
        pos = pos + Vector3D(P.gradientShift * P.gradientPeriod, 0.0f, 0.0f);

        // --- Phase along the gradient
        float phase = 0.0f; // in [0, period)
        if (P.isRadial) {
            const float r = std::sqrt(pos.X * pos.X + pos.Y * pos.Y);
            phase = std::fabs(std::fmod(r, P.gradientPeriod));
        } else {
            phase = std::fabs(std::fmod(pos.X, P.gradientPeriod));
        }

        // map 0..period -> 0..N
        const float t     = Mathematics::Map(phase, 0.0f, P.gradientPeriod, 0.0f, float(N));
        const uint8_t i0  = (uint8_t)std::floor(t);
        const uint8_t i1  = (i0 + 1u >= (uint8_t)N) ? 0u : (uint8_t)(i0 + 1u);

        if (P.isStepped) {
            return P.colors[i0];
        }

        const float mu = Mathematics::Map(t, float(i0), float(i0 + 1u), 0.0f, 1.0f);
        return RGBColor::InterpolateColors(P.colors[i0], P.colors[i1], mu);
    }
};
