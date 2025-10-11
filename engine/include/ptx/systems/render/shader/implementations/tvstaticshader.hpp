// tvstaticshader.hpp
#pragma once

#include <cmath>
#include <utility>
#include <vector>
#include "../../../../registry/reflect_macros.hpp"

#include "../ishader.hpp"
#include "../../material/materialt.hpp"

#include "tvstaticparams.hpp"

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/color/gradientcolor.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/signal/noise/simplexnoise.hpp"

/**
 * @file tvstaticshader.hpp
 * @brief TV static shader combining simplex noise, scanlines, and color bars.
 *
 * Pipeline:
 *   base = Gradient(noise(simplex))
 *   scan = Gradient(u) with period/shift (90° rotation via axis choice)
 *   mix1 = lerp(base, base*scan, scanOpacity)         // multiply blend with opacity
 *   bars = color bars overlay                          // lighten/overlay
 *   out  = max(mix1, bars)                             // lighten
 *
 */
class TVStaticShader final : public IShader {
public:
    /** @brief Construct with default simplex noise seed/state. */
    TVStaticShader() : noise_(0) {}

    /**
     * @brief Shade a surface point using TV static composition.
     * @param sp Surface properties (position in world/screen space).
     * @param m  Material instance expected to be MaterialT<TVStaticParams, TVStaticShader>.
     * @return Final RGB color after noise/scan/bars composition.
     */
    RGBColor Shade(const SurfaceProperties& sp, const IMaterial& m) const override {
        using MatBase = MaterialT<TVStaticParams, TVStaticShader>;
        const auto& P = m.As<MatBase>();

        // --- Build hue-shifted gradients ---
        const std::size_t noiseCount = P.noiseSpectrum.size();
        const std::size_t scanCount  = P.scanSpectrum.size();
        if (noiseCount == 0 || scanCount == 0) {
            return RGBColor();
        }

        std::vector<RGBColor> noiseKeys(noiseCount);
        for (std::size_t i = 0; i < noiseCount; ++i) {
            noiseKeys[i] = RGBColor(P.noiseSpectrum[i]).HueShift(P.noiseHueDeg);
        }
        GradientColor gNoise(std::move(noiseKeys), /*stepped=*/true);

        const RGBColor* scanPtr = P.scanSpectrum.empty() ? nullptr : P.scanSpectrum.data();
        GradientColor gScan(scanPtr, scanCount, /*stepped=*/false);

        // --- 1) Base noise color (simplex -> gradient) ---
        Vector3D ns(sp.position.X * P.noiseScale.X,
                    sp.position.Y * P.noiseScale.Y,
                    P.noiseZ       * P.noiseScale.Z);

        float n = noise_.GetNoise(ns);              // typically [-1, 1]
        float s = (n * 0.5f + 0.5f);                // [0, 1]
        float t = s / P.noiseGradientPeriod;        // repeat
        t -= Mathematics::FFloor(t);                // fract
        RGBColor base = gNoise.GetColorAt(t);

        // --- 2) Scanline mod color ---
        // rotate 90° by sampling along X (so stripes go horizontally)
        float scanU = (sp.position.X / P.scanGradientPeriod) + P.scanShift;
        scanU -= Mathematics::FFloor(scanU);
        RGBColor scanCol = gScan.GetColorAt(scanU);

        // Multiply blend with opacity: mix(base, base*scan, opacity)
        RGBColor mul(base.R * scanCol.R / 255.0f,
                     base.G * scanCol.G / 255.0f,
                     base.B * scanCol.B / 255.0f);
        RGBColor mix1 = Lerp(base, mul, P.scanMultiplyOpacity);

        // --- 3) Color bars overlay
        RGBColor bars = ColorBars(sp.position, P);

        // Lighten blend
        RGBColor out(std::fmax(mix1.R, bars.R),
                     std::fmax(mix1.G, bars.G),
                     std::fmax(mix1.B, bars.B));

        return out;
    }

private:
    SimplexNoise noise_; ///< Simplex noise generator.

    /** @brief Linear interpolation between two colors. */
    static RGBColor Lerp(const RGBColor& a, const RGBColor& b, float t) {
        return RGBColor(a.R + (b.R - a.R) * t,
                        a.G + (b.G - a.G) * t,
                        a.B + (b.B - a.B) * t);
    }

    /**
     * @brief PAL/NTSC-style color bars with soft edges, within a rectangular region.
     * @param pos Position to shade (expects XY components).
     * @param P   Parameter block.
     * @return Color bars contribution at @p pos.
     */
    static RGBColor ColorBars(const Vector3D& pos, const TVStaticParams& P) {
        // local coords centered at barsCenter
        float x = pos.X - P.barsCenter.X;
        float y = pos.Y - P.barsCenter.Y;

        // Tile seven bars horizontally across barsSize.X
        const int   kNumBars = 7;
        const float halfW    = P.barsSize.X * 0.5f;
        const float halfH    = P.barsSize.Y * 0.5f;

        if (x < -halfW || x > halfW || y < -halfH || y > halfH) {
            return RGBColor(0,0,0);
        }

        // Normalize to [0,1] across width then to bar index
        float u   = (x + halfW) / (P.barsSize.X);
        int   idx = (int)Mathematics::FFloor(u * kNumBars);
        if (idx < 0) {
            idx = 0;
        }
        if (idx >= kNumBars) {
            idx = kNumBars - 1;
        }

        // canonical bar colors
        static const RGBColor kBars[kNumBars] = {
            RGBColor(191,191,191), // gray
            RGBColor(191,191,0),   // yellow
            RGBColor(0,191,191),   // cyan
            RGBColor(0,191,0),     // green
            RGBColor(191,0,191),   // magenta
            RGBColor(191,0,0),     // red
            RGBColor(0,0,191)      // blue
        };

        // Soft vertical mask (fade top/bottom)
        float v  = (y + halfH) / (P.barsSize.Y);  // 0..1
        float vm = 1.0f - SmoothStep(0.0f, 0.05f, v) - (1.0f - SmoothStep(0.95f, 1.0f, v));
        vm = Mathematics::Constrain(vm, 0.0f, 1.0f);

        // Soft horizontal edges between bars
        float barU   = u * kNumBars - (float)idx;      // 0..1 within current bar
        float edge   = EdgeMask(barU, P.barsSoftness / P.barsSize.X);
    RGBColor col = RGBColor(kBars[idx]).HueShift(P.barsHueDeg);

        return RGBColor(col.R * vm * edge,
                        col.G * vm * edge,
                        col.B * vm * edge);
    }

    /** @brief SmoothStep interpolation helper in [a,b]. */
    static float SmoothStep(float a, float b, float x) {
        float t = Mathematics::Constrain((x - a) / (b - a), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    /** @brief Edge softening mask within a unit bar coordinate. */
    static float EdgeMask(float u, float softness) {
        float e = 1.0f - (SmoothStep(0.0f, softness, u) +
                          (1.0f - SmoothStep(1.0f - softness, 1.0f, u)));
        return Mathematics::Constrain(e, 0.0f, 1.0f);
    }

    PTX_BEGIN_FIELDS(TVStaticShader)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(TVStaticShader)
        PTX_METHOD_AUTO(TVStaticShader, Shade, "Shade")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(TVStaticShader)
        PTX_CTOR0(TVStaticShader)
    PTX_END_DESCRIBE(TVStaticShader)

};
