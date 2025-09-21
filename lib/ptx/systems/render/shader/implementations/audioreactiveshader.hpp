#pragma once

#include <cstddef>
#include <cmath>

#include "../ishader.hpp"
#include "../../material/materialt.hpp"

#include "audioreactiveparams.hpp"

#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/color/gradientcolor.hpp"

/**
 * @file audioreactiveshader.hpp
 * @brief Audio-reactive gradient shader (template).
 *
 * Renders either a horizontal bar visualization (non-circular) or a circular ring visualization
 * (circular mode), driven by B audio samples and an N-key color spectrum.
 *
 * @tparam N Number of spectrum keys (gradient control points).
 * @tparam B Number of audio samples processed per frame.
 */

/**
 * @class AudioReactiveShaderT
 * @brief Shader that visualizes audio samples using a hue-shifted gradient.
 *
 * - Non-circular: X maps to sample bin; Y compares against an interpolated “height”.
 * - Circular: XY is mapped to polar; draws a band near a target radius.
 * - Spectrum keys are hue-shifted and used to build a continuous gradient.
 */
template <std::size_t N, std::size_t B>
class AudioReactiveShaderT final : public IShader {
public:
    /**
     * @brief Shade a surface point using audio-reactive logic.
     * @param sp Surface properties (expects @c sp.position to be valid).
     * @param m  Bound material, expected to be @c MaterialT<AudioReactiveParamsT<N,B>, AudioReactiveShaderT<N,B>>.
     * @return RGB color for the point; black when outside the band or without samples.
     *
     * @details
     * - Builds a hue-shifted gradient from the material’s @c spectrum[N].
     * - Transforms @c sp.position into the material’s local space (offset/rotation).
     * - Non-circular mode compares @c q.Y against an interpolated height from samples.
     * - Circular mode converts to polar and draws a band around @c radius with audio-driven thickness.
     */
    RGBColor Shade(const SurfaceProperties& sp, const IMaterial& m) const override {
        using MatBase = MaterialT<AudioReactiveParamsT<N, B>, AudioReactiveShaderT<N, B>>;
        const auto& P = m.As<MatBase>();

        if (!P.samples) return RGBColor(0,0,0);

        // Build hue-shifted gradient
        RGBColor keys[N];
        for (std::size_t i = 0; i < N; ++i) {
            keys[i] = RGBColor(P.spectrum[i]).HueShift(P.hueDeg);
        }
        GradientColor<N> grad(keys, /*stepped*/false);

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
            float tX = rPos.X;
            polar.X = std::atan2(rPos.Y, rPos.X) / (2.0f * Mathematics::MPI) * P.sizeHalf.Y; // angle scaled to Y-range
            polar.Y = std::sqrt(tX * tX + rPos.Y * rPos.Y);                                   // radius
        }

        // Choose coordinate set (non-circular: rPos; circular: polar)
        const Vector2D& q = P.circular ? polar : rPos;

        // Map X -> bin index (reverse mapping per original)
        const float fx = Mathematics::Map(q.X, -P.sizeHalf.X, P.sizeHalf.X, float(B), 0.0f);
        int   x0 = int(std::floor(fx));
        x0 = Mathematics::Constrain(x0, 0, int(B - 1));
        int   x1 = Mathematics::Constrain(x0 + 1, 0, int(B - 1));
        const float t = fx - float(x0); // between x0 and x1

        // Interpolated height from either bounced or raw data
        const float s0 = P.bounce ? P.bounceData[x0] : P.samples[x0];
        const float s1 = P.bounce ? P.bounceData[x1] : P.samples[x1];
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

};
