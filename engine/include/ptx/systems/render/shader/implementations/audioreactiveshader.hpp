#pragma once

#include <cstddef>
#include <cmath>
#include <vector>
#include <utility>

#include "../ishader.hpp"
#include "../../material/materialt.hpp"
#include "../../../../registry/reflect_macros.hpp"

#include "audioreactiveparams.hpp"

#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/color/gradientcolor.hpp"

/**
 * @file audioreactiveshader.hpp
 * @brief Audio-reactive gradient shader with runtime-sized spectrum and sample buffers.
 */

/**
 * @class AudioReactiveShader
 * @brief Shader that visualizes audio samples using a hue-shifted gradient.
 *
 * - Non-circular: X maps to sample bin; Y compares against an interpolated “height”.
 * - Circular: XY is mapped to polar; draws a band near a target radius.
 * - Spectrum keys are hue-shifted and used to build a continuous gradient.
 */
class AudioReactiveShader final : public IShader {
public:
    RGBColor Shade(const SurfaceProperties& sp, const IMaterial& m) const override {
        using MatBase = MaterialT<AudioReactiveParams, AudioReactiveShader>;
        const auto& P = m.As<MatBase>();

        const std::size_t colorCount  = P.spectrum.size();
        const std::size_t sampleCount = P.SampleCount();
        if (colorCount == 0 || sampleCount == 0) {
            return RGBColor(0, 0, 0);
        }

        // No raw data available when bounce is disabled -> nothing to draw.
        if (!P.bounce && !P.samples) {
            return RGBColor(0, 0, 0);
        }

        // Build hue-shifted gradient
        std::vector<RGBColor> keys(colorCount);
        for (std::size_t i = 0; i < colorCount; ++i) {
            keys[i] = RGBColor(P.spectrum[i]).HueShift(P.hueDeg);
        }
        GradientColor grad(std::move(keys), /*stepped*/false);

        // Rotate/translate to local space
        Vector2D p(sp.position.X, sp.position.Y);
        Vector2D rPos;
        if (Mathematics::IsClose(P.angleDeg, 0.0f, 0.1f)) {
            rPos = p - P.offset;
        } else {
            rPos = p.Rotate(P.angleDeg, P.offset) - P.offset;
        }

        // Bounds
        if (rPos.X < -P.sizeHalf.X || rPos.X > P.sizeHalf.X) return RGBColor();
        if (rPos.Y < -P.sizeHalf.Y || rPos.Y > P.sizeHalf.Y) return RGBColor();

        // Optionally convert to polar for circular mode
        Vector2D polar = rPos;
        if (P.circular) {
            const float tX = rPos.X;
            polar.X = std::atan2(rPos.Y, rPos.X) / (2.0f * Mathematics::MPI) * P.sizeHalf.Y; // angle scaled to Y-range
            polar.Y = std::sqrt(tX * tX + rPos.Y * rPos.Y);                                   // radius
        }

        // Choose coordinate set (non-circular: rPos; circular: polar)
        const Vector2D& q = P.circular ? polar : rPos;

        const float sampleCountF = static_cast<float>(sampleCount);
        const float fx = Mathematics::Map(q.X, -P.sizeHalf.X, P.sizeHalf.X, sampleCountF, 0.0f);
        int x0 = static_cast<int>(std::floor(fx));
        const int maxIndex = static_cast<int>(sampleCount > 0 ? sampleCount - 1 : 0);
        x0 = Mathematics::Constrain(x0, 0, maxIndex);
        int x1 = Mathematics::Constrain(x0 + 1, 0, maxIndex);
        const float t = fx - static_cast<float>(x0); // between x0 and x1

        const auto src = P.bounce ? P.bounceData.data() : P.samples;
        if (!src) {
            return RGBColor(0, 0, 0);
        }

        const float s0 = src[x0];
        const float s1 = src[x1];
        const float height = Mathematics::CosineInterpolation(s0, s1, t); // 0..1

        if (!P.circular) {
            // Non-circular branch
            const float yColor = Mathematics::Map(q.Y, 0.0f, P.sizeHalf.Y, 1.0f, 0.0f);
            const float inside = 1.0f - (height * 4.0f + 0.15f) - yColor;
            if (inside < 0.0f) {
                float g = 1.0f - height - yColor;
                g = Mathematics::Constrain(g, 0.0f, 1.0f);
                return grad.GetColorAt(g);
            }
            return RGBColor(0,0,0);
        } else {
            // Circular branch: draw ring near target radius with audio thickness
            const float band = height * 150.0f;
            const float r    = q.Y;
            const float edge = P.radius - 5.0f;

            if ( (r + band > edge) && (r - band < edge) ) {
                const float yColor = Mathematics::Map(r, 0.0f, P.sizeHalf.Y, 1.0f, 0.0f);
                float g = 1.0f + height - yColor;
                g = Mathematics::Constrain(g, 0.0f, 1.0f);
                return grad.GetColorAt(g);
            }
            return RGBColor(0,0,0);
        }
    }

    PTX_BEGIN_FIELDS(AudioReactiveShader)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(AudioReactiveShader)
        PTX_METHOD_AUTO(AudioReactiveShader, Shade, "Shade")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(AudioReactiveShader)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(AudioReactiveShader)
};
