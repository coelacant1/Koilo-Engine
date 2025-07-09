#pragma once
#include "../../material/imaterial.hpp"
#include "../../material/materialt.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/signal/noise/simplexnoise.hpp"
#include "../../../../core/color/gradientcolor.hpp"

// Parameter block  ------------------------------------------------------------
struct ProceduralNoiseParams {
    static constexpr std::size_t kColors = 4;         // gradient resolution

    RGBColor spectrum[kColors] = {                    // default = black-white stripes
        {0,0,0}, {255,255,255}, {0,0,0}, {255,255,255}
    };

    Vector3D noiseScale        {0.01f, 0.01f, 0.01f}; // freq per axis
    float    simplexDepth      = 0.0f;                // Z slice for time-animation
    float    gradientPeriod    = 1.0f;                // repeat every N noise units
    float    hueShiftAngleDeg  = 0.0f;                // rainbow phase-shift
};

// Shader  ---------------------------------------------------------------------
class ProceduralNoiseShader final : public IShader{
public:
    RGBColor Shade(const SurfaceProperties& sp,
                   const IMaterial&         m) const override {
        // Compile-time-safe down-cast
        using NoiseMat = MaterialT<ProceduralNoiseParams, 
                                   ProceduralNoiseShader>;

        const auto& mat = m.As<NoiseMat>();

        // --- build shifted colour ramp ---------------------------------------
        RGBColor shifted[ProceduralNoiseParams::kColors];
        for (std::size_t i = 0; i < ProceduralNoiseParams::kColors; ++i)
            shifted[i] = mat.spectrum[i].HueShift(mat.hueShiftAngleDeg);

        GradientColor<ProceduralNoiseParams::kColors> gradient(*shifted, mat.gradientPeriod, false);

        // --- simplex noise ---------------------------------------------------
        SimplexNoise noise{/*octaves*/4};
        noise.SetScale(mat.noiseScale);
        noise.SetZPosition(mat.simplexDepth);

        float n = noise.Get(sp.position);             // 0‥1
        return gradient.Evaluate(n);                  // sample ramp
    }
};
