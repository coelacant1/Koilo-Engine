#pragma once

#include <cstddef>
#include <cmath>
#include "../../../../registry/reflect_macros.hpp"

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
 * @brief Stateless shader that samples a linear or radial gradient with runtime-sized keys.
 *
 * Uses the bound material’s @ref GradientParams to compute a phase along +X (linear)
 * or radial distance (radial), applies optional rotation about a configurable origin,
 * and returns either stepped or interpolated color between adjacent keys.
 */

class GradientShader final : public IShader {
public:
    /**
     * @brief Shade a surface point by evaluating the configured gradient.
     * @param surf Surface properties (position used; XY plane evaluation).
    * @param m    Bound material; expected to be @c MaterialT<GradientParams, GradientShader>.
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
        using MatBase = MaterialT<GradientParams, GradientShader>;
        const auto& P = m.As<MatBase>();

        const std::size_t colorCount = P.colors.size();
        if (colorCount == 0) {
            return RGBColor(0, 0, 0);
        }

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
        const float maxPeriod = (P.gradientPeriod > 0.00001f) ? P.gradientPeriod : 1.0f;
        const float mapped    = Mathematics::Map(phase, 0.0f, maxPeriod, 0.0f, static_cast<float>(colorCount));
        float t = std::fmod(mapped, static_cast<float>(colorCount));
        if (t < 0.0f) t += static_cast<float>(colorCount);

        std::size_t i0 = static_cast<std::size_t>(std::floor(t));
        if (i0 >= colorCount) i0 = colorCount - 1;
        const std::size_t i1 = (colorCount == 1) ? 0 : ((i0 + 1) % colorCount);

        if (P.isStepped || colorCount == 1) {
            return P.colors[i0];
        }

        const float mu = t - static_cast<float>(i0);
        return RGBColor::InterpolateColors(P.colors[i0], P.colors[i1], mu);
    }

    PTX_BEGIN_FIELDS(GradientShader)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(GradientShader)
        PTX_METHOD_AUTO(GradientShader, Shade, "Shade")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(GradientShader)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(GradientShader)

};
