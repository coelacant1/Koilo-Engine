// oscilloscopeshader.hpp
#pragma once

#include <cstddef>
#include <cmath>

#include "../ishader.hpp"
#include "../../material/materialt.hpp"
#include "oscilloscopeparams.hpp"

#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/color/gradientcolor.hpp"

/**
 * @file oscilloscopeshader.hpp
 * @brief Shader that renders a thin oscilloscope trace from B samples using an N-color gradient.
 *
 * @tparam N Number of gradient colors.
 * @tparam B Number of sample bins (FFT or time-domain).
 */
template <size_t N, size_t B>
class OscilloscopeShaderT final : public IShader {
public:
    /**
     * @brief Shade a point against the oscilloscope trace.
     * @param sp Surface properties (position is used).
     * @param m  Bound material; expected to be @c MaterialT<OscilloscopeParamsT<N,B>, OscilloscopeShaderT<N,B>>.
     * @return Trace color if the point lies on the line; otherwise transparent/black.
     *
     * @details
     * - Applies rotation around the parameter @ref OscilloscopeParamsT::offset.
     * - Maps X to the [0..B-1] bin range with cosine interpolation between adjacent bins.
     * - Computes a thin strip at the interpolated height with thickness proportional to @ref sizeHalf.
     * - Colors the strip via an @c N-key gradient with an additional hue shift.
     */
    RGBColor Shade(const SurfaceProperties& sp, const IMaterial& m) const override {
        using MatBase = MaterialT<OscilloscopeParamsT<N, B>, OscilloscopeShaderT<N, B>>;
        const auto& P = m.As<MatBase>();

        if (!P.samples) return RGBColor(0,0,0);

        // Build a hue-shifted gradient for this call.
        RGBColor shifted[N];
        for (size_t i = 0; i < N; ++i) {
            shifted[i] = RGBColor(P.spectrum[i]).HueShift(P.hueDeg);
        }
        GradientColor<N> gradient(shifted, false);

        // Rotate/translate to local oscilloscope space.
        Vector2D p2(sp.position.X, sp.position.Y);
        Vector2D rPos;
        if (Mathematics::IsClose(P.angleDeg, 0.0f, 0.1f)) {
            rPos = p2 - P.offset;
        } else {
            rPos = p2.Rotate(P.angleDeg, P.offset) - P.offset;
        }

        // Bounds check (half extents).
        if (rPos.X < -P.sizeHalf.X || rPos.X > P.sizeHalf.X) return RGBColor();
        if (rPos.Y < -P.sizeHalf.Y || rPos.Y > P.sizeHalf.Y) return RGBColor();

        // Map X to [0 .. B-1].
        const float fx = Mathematics::Map(rPos.X, -P.sizeHalf.X, P.sizeHalf.X, 0.0f, float(B - 1));
        const int   x0 = Mathematics::Constrain(int(Mathematics::FFloor(fx)), 0, int(B - 1));
        const int   x1 = Mathematics::Constrain(x0 + 1,                         0, int(B - 1));
        const float t  = fx - float(x0);

        // Sample two bins and cosine-interpolate.
        const float s0  = P.samples[x0];
        const float s1  = P.samples[x1];
        const float s0m = Mathematics::Map(s0, P.minValue, P.maxValue, 0.0f, P.heightClamp);
        const float s1m = Mathematics::Map(s1, P.minValue, P.maxValue, 0.0f, P.heightClamp);
        const float height = Mathematics::CosineInterpolation(s0m, s1m, t); // 0..heightClamp

        // Compute Y placement and thickness.
        const float top       = height * P.sizeHalf.Y;
        const float thickness = P.lineThickness * P.sizeHalf.Y;

        // Within the thin trace band?
        const bool onTrace = (rPos.Y <  top) && (rPos.Y > (top - thickness));
        if (!onTrace) return RGBColor(0,0,0);

        // Gradient coordinate (0..1).
        const float yColor = Mathematics::Map(rPos.Y, 0.0f, P.sizeHalf.Y, 1.0f, 0.0f);
        float g = 1.0f + height - yColor;
        g = Mathematics::Constrain(g, 0.0f, 1.0f);

        return gradient.GetColorAt(g);
    }
};
