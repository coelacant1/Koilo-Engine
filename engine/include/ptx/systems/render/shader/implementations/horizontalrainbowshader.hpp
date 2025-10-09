#pragma once

#include <cmath>

#include "../ishader.hpp"
#include "../../material/materialt.hpp"
#include "../../../../registry/reflect_macros.hpp"
#include "horizontalrainbowparams.hpp"

#include "../../../../core/color/gradientcolor.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

/**
 * @file horizontalrainbowshader.hpp
 * @brief Shader that generates a horizontal rainbow gradient using runtime spectrum keys.
 *
 * Workflow:
 * - Builds a linear @ref GradientColor from the material’s @c spectrum.
 * - Transforms the surface position by @c positionOffset and @c rotationDeg (around origin).
 * - Samples the gradient along the X axis with wrap using @c gradientPeriod.
 *
 */

/**
 * @class HorizontalRainbowShader
 * @brief Rainbow shader sampling a periodic horizontal gradient.
 *
 * The shade operation:
 * 1. Reads the owning material via @c IMaterial::As<MaterialT<...>>() to access parameters.
 * 2. Constructs a linear gradient from @c spectrum entries.
 * 3. Applies offset/rotation to the XY position from @ref SurfaceProperties.
 * 4. Projects X into [0, @c gradientPeriod) and normalizes to [0,1] for gradient lookup.
 *
 * Assumes degrees for rotation and wraps negative X using @c std::fmod rules.
 */
class HorizontalRainbowShader final : public IShader {
public:
    /**
     * @brief Shade a surface point using a periodic horizontal gradient.
     * @param sp Surface properties (position XYZ, normals, etc.).
     * @param m  Bound material; expected to be @c MaterialT<HorizontalRainbowParams, HorizontalRainbowShader>.
     * @return Interpolated @ref RGBColor at the computed gradient coordinate.
     */
    RGBColor Shade(const SurfaceProperties& sp, const IMaterial& m) const override {
        using MatBase = MaterialT<HorizontalRainbowParams, HorizontalRainbowShader>;
        const auto& P = m.As<MatBase>();

        const std::size_t spectrumCount = P.spectrum.size();
        if (spectrumCount == 0) {
            return RGBColor();
        }

        // Build gradient (linear)
        GradientColor grad(
            P.spectrum.data(),
            spectrumCount,
            /*stepped*/false
        );

        // Local 2D position with offset & rotation (about origin)
        Vector2D pos(sp.position.X, sp.position.Y);
        pos = pos - P.positionOffset;
        if (!Mathematics::IsClose(P.rotationDeg, 0.0f, 0.001f)) {
            pos = pos.Rotate(P.rotationDeg, Vector2D(0.0f, 0.0f));
        }

        // Project to gradient coordinate along X
        const float period = (P.gradientPeriod > 0.00001f) ? P.gradientPeriod : 1.0f;
        float x = std::fmod(pos.X, period);
        if (x < 0.0f) x += period;
        const float t = x / period; // 0..1

        return grad.GetColorAt(t);
    }

    PTX_BEGIN_FIELDS(HorizontalRainbowShader)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(HorizontalRainbowShader)
        PTX_METHOD_AUTO(HorizontalRainbowShader, Shade, "Shade")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(HorizontalRainbowShader)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(HorizontalRainbowShader)
};
