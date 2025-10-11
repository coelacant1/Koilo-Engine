// spectrumanalyzeshader.hpp
#pragma once

#include <cstddef>
#include <cmath>
#include <vector>
#include "../../../../registry/reflect_macros.hpp"

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
 * @brief Shader for rendering a spectrum analyzer using runtime-sized gradients over dynamic bins.
 *
 * Maps X to a bin index, cosine-interpolates between adjacent bins for height, supports mirror/flip Y shaping,
 * and colors with a hue-shifted gradient built from @ref SpectrumAnalyzerParams::spectrum.
 */

class SpectrumAnalyzerShader final : public IShader {
public:
    /**
     * @brief Shade a surface point using spectrum analyzer rules.
     * @param sp Surface properties (position in world space expected).
     * @param m  Material; expected to be a @c MaterialT<SpectrumAnalyzerParams, SpectrumAnalyzerShader>.
     * @return RGB color at the given point.
     */
    RGBColor Shade(const SurfaceProperties& sp, const IMaterial& m) const override {
        using MatBase = MaterialT<SpectrumAnalyzerParams, SpectrumAnalyzerShader>;
        const auto& P = m.As<MatBase>();

        const std::size_t spectrumCount = P.SpectrumCount();
        const std::size_t binCount      = P.BinCount();
        if (spectrumCount == 0 || binCount == 0) {
            return RGBColor(0,0,0);
        }

        const float* sampleSource = nullptr;
        if (P.bounce) {
            sampleSource = P.BounceData();
            if (!sampleSource) {
                return RGBColor(0,0,0);
            }
        } else {
            sampleSource = P.samples;
            if (!sampleSource) {
                return RGBColor(0,0,0);
            }
        }

        // Build hue-shifted gradient keys
        std::vector<RGBColor> keys(spectrumCount);
        const RGBColor* spectrumData = P.SpectrumData();
        for (std::size_t i = 0; i < spectrumCount; ++i) {
            const RGBColor base = spectrumData ? spectrumData[i] : RGBColor();
            keys[i] = RGBColor(base).HueShift(P.hueDeg);
        }
        GradientColor grad(std::move(keys), /*stepped*/false);

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
    const float fx = Mathematics::Map(rPos.X, -P.sizeHalf.X, P.sizeHalf.X, static_cast<float>(binCount), 0.0f);
    int x0 = static_cast<int>(std::floor(fx));
    const int maxIndex = static_cast<int>(binCount > 0 ? binCount - 1 : 0);
    x0 = Mathematics::Constrain(x0, 0, maxIndex);
    int x1 = Mathematics::Constrain(x0 + 1, 0, maxIndex);
        const float t = fx - float(x0);

        // Interpolated height from bounced or raw data
    const float s0 = sampleSource[x0];
    const float s1 = sampleSource[x1];
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

    PTX_BEGIN_FIELDS(SpectrumAnalyzerShader)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(SpectrumAnalyzerShader)
        PTX_METHOD_AUTO(SpectrumAnalyzerShader, Shade, "Shade")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(SpectrumAnalyzerShader)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(SpectrumAnalyzerShader)

};
