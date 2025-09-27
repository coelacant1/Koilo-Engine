// spectrumanalyzeshader.hpp
#pragma once

#include <cstddef>
#include <cmath>

#include "../ishader.hpp"
#include "../../material/materialt.hpp"
#include "spectrumanalyzerparams.hpp"

#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/color/gradientcolor.hpp"

/**
 * @file spectrumanalyzeshader.hpp
 * @brief Shader for rendering a spectrum analyzer using N-key gradients over B bins.
 *
 * Maps X to a bin index, cosine-interpolates between adjacent bins for height, supports
 * mirror/flip Y shaping, and colors with a hue-shifted gradient built from @ref SpectrumAnalyzerParamsT::spectrum.
 *
 * @tparam N Gradient key count.
 * @tparam B Number of spectrum bins.
 */

/**
 * @brief Shader that renders a spectrum analyzer bar field.
 * @tparam N Gradient key count.
 * @tparam B Number of spectrum bins.
 */
template <std::size_t N, std::size_t B>
class SpectrumAnalyzerShaderT final : public IShader {
public:
    /**
     * @brief Shade a surface point using spectrum analyzer rules.
     * @param sp Surface properties (position in world space expected).
     * @param m  Material; expected to be a @c MaterialT<SpectrumAnalyzerParamsT<N,B>, SpectrumAnalyzerShaderT<N,B>>.
     * @return RGB color at the given point.
     */
    RGBColor Shade(const SurfaceProperties& sp, const IMaterial& m) const override {
        using MatBase = MaterialT<SpectrumAnalyzerParamsT<N, B>, SpectrumAnalyzerShaderT<N, B>>;
        const auto& P = m.As<MatBase>();

        if (!P.samples) return RGBColor(0,0,0);

        // Build hue-shifted gradient keys
        RGBColor keys[N];
        for (std::size_t i = 0; i < N; ++i) {
            keys[i] = RGBColor(P.spectrum[i]).HueShift(P.hueDeg);
        }
        GradientColor<N> grad(keys, /*stepped*/false);

        // Transform to local space (rotate about offset)
        Vector2D p(sp.position.X, sp.position.Y);
        Vector2D rPos;
        if (Mathematics::IsClose(P.angleDeg, 0.0f, 0.1f)) {
            rPos = p - P.offset;
        } else {
            rPos = p.Rotate(P.angleDeg, P.offset) - P.offset;
        }

        // Bounds check
        if (rPos.X < -P.sizeHalf.X || rPos.X > P.sizeHalf.X) return RGBColor();
        if (rPos.Y < -P.sizeHalf.Y || rPos.Y > P.sizeHalf.Y) return RGBColor();

        // Map X -> bin index (left = higher index)
        const float fx = Mathematics::Map(rPos.X, -P.sizeHalf.X, P.sizeHalf.X, float(B), 0.0f);
        int x0 = int(std::floor(fx));
        x0 = Mathematics::Constrain(x0, 0, int(B - 1));
        int x1 = Mathematics::Constrain(x0 + 1, 0, int(B - 1));
        const float t = fx - float(x0);

        // Interpolated height from bounced or raw data
        const float s0 = P.bounce ? P.bounceData[x0] : P.samples[x0];
        const float s1 = P.bounce ? P.bounceData[x1] : P.samples[x1];
        float height   = Mathematics::CosineInterpolation(s0, s1, t); // 0..1
        height *= P.heightScale;

        // Y ramp with mirror/flip options
        float yColor;
        if (P.mirrorY) {
            yColor = Mathematics::Map(std::fabs(rPos.Y), P.sizeHalf.Y, 0.0f, 1.0f, 0.0f);
        } else {
            yColor = Mathematics::Map(rPos.Y, -P.sizeHalf.Y, P.sizeHalf.Y, 1.0f, 0.0f);
        }
        if (P.flipY) yColor = 1.0f - yColor;

        // Draw if within bar height
        if (yColor <= height) {
            float g = 1.0f - height - yColor;                // gradient coordinate
            g = Mathematics::Constrain(g, 0.0f, 1.0f);
            return grad.GetColorAt(g);
        }
        return RGBColor(0,0,0);
    }

};
