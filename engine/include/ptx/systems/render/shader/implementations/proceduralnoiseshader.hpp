// proceduralnoiseshader.hpp
#pragma once

#include <cstddef>
#include <cmath>

#include "../ishader.hpp"
#include "../../material/materialt.hpp"
#include "proceduralnoiseparams.hpp"

#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/color/gradientcolor.hpp"
#include "../../../../core/signal/noise/simplexnoise.hpp"

/**
 * @file proceduralnoiseshader.hpp
 * @brief Simplex noise -> [0,1] -> periodic mapping -> gradient sampling with optional hue shift.
 *
 * @tparam N Number of colors in the gradient spectrum.
 */
template <size_t N>
class ProceduralNoiseShaderT final : public IShader {
public:
    /**
     * @brief Shade a surface point using periodic simplex noise and a gradient spectrum.
     * @param sp Surface properties (uses @c sp.position).
     * @param m  Bound material expected to be @c MaterialT<ProceduralNoiseParamsT<N>, ProceduralNoiseShaderT<N>>.
     * @return RGB color sampled from the hue-shifted gradient at the periodic noise coordinate.
     */
    RGBColor Shade(const SurfaceProperties& sp, const IMaterial& m) const override {
        using NoiseMat = MaterialT<ProceduralNoiseParamsT<N>, ProceduralNoiseShaderT<N>>;
        const auto& P = m.As<NoiseMat>();

        // Build a hue-shifted spectrum for this shade call
        RGBColor shifted[N];
        for (std::size_t i = 0; i < N; ++i) {
            shifted[i] = RGBColor(P.spectrum[i]).HueShift(P.hueShiftAngleDeg);
        }
        // false => linear (non-stepped) gradient
        GradientColor<N> gradient(shifted, false);

        // Scale input position and add slice depth on Z
        Vector3D p = sp.position;
        p.X *= P.noiseScale.X;
        p.Y *= P.noiseScale.Y;
        p.Z  = p.Z * P.noiseScale.Z + P.simplexDepth;

        // Simplex noise in [-1,1] -> remap to [0,1]
        static SimplexNoise noise(/*seed*/0);
        const float n01 = 0.5f * (noise.GetNoise(p) + 1.0f);

        // Repeat by gradientPeriod (guard against zero/neg)
        const float period = (P.gradientPeriod > 0.00001f) ? P.gradientPeriod : 1.0f;
        const float cycles = n01 / period;
        const float t = cycles - Mathematics::FFloor(cycles);  // fract

        return gradient.GetColorAt(t);
    }

};
