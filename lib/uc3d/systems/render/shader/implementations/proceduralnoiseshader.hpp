#pragma once

#include "proceduralnoiseparams.hpp"
#include "../../material/imaterial.hpp"
#include "../../material/materialt.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/signal/noise/simplexnoise.hpp"
#include "../../../../core/color/gradientcolor.hpp"

class ProceduralNoiseShader final : public IShader{
private:
    SimplexNoise noise{0};

public:
    RGBColor Shade(const SurfaceProperties& sp,
                   const IMaterial&         m) override {

        using NoiseMat = MaterialT<ProceduralNoiseParams, 
                                   ProceduralNoiseShader>;

        const auto& mat = m.As<NoiseMat>();

        RGBColor shifted[ProceduralNoiseParams::kColors];
        for (std::size_t i = 0; i < ProceduralNoiseParams::kColors; ++i) {
            shifted[i] = RGBColor(mat.spectrum[i]).HueShift(mat.hueShiftAngleDeg);
        }

        GradientColor<ProceduralNoiseParams::kColors> gradient(shifted, false);

        float n = noise.GetNoise(sp.position);
        float scaled_n = n / mat.gradientPeriod;
        float t = scaled_n - Mathematics::FFloor(scaled_n);

        return gradient.GetColorAt(t);
    }
};
