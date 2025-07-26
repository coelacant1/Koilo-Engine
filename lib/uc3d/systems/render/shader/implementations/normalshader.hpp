#pragma once

#include "normalparams.hpp"
#include "../../material/imaterial.hpp"
#include "../../material/materialt.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

class NormalShader final : public IShader {
public:
    RGBColor Shade(const SurfaceProperties& sp,
                   const IMaterial&         m) override {
        // Compile-time-safe down-cast
        using NoiseMat = MaterialT<NormalParams, 
                                   NormalShader>;

        const auto& mat = m.As<NoiseMat>();

        Vector3D normalL = sp.normal.UnitSphere();

        normalL = (normalL + 1.0f) * 0.5f * 255.0f;

        return RGBColor(normalL);
    }
};
