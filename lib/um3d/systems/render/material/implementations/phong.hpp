#pragma once

#include "../materialt.hpp"
#include "../../shader/ishader.hpp"
#include "../../../../core/color/color.hpp"
#include "../../../../core/math/vector3d.hpp"

struct PhongParams {
    RGBColor albedo   {1,1,1};
    float shininess = 32.0f;
};

class PhongShader final : public IShader {
public:
    RGBColor Shade(const SurfaceProperties& s,
                   const IMaterial& m) const override;
};

inline const PhongShader g_PhongShader;
inline constexpr const IShader* kPhongShaderPtr = &g_PhongShader;

using PhongMaterial = MaterialT<PhongParams, PhongShader>;

PhongMaterial pM = PhongMaterial();