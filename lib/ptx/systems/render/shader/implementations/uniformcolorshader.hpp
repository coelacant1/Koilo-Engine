// uniformcolorshader.hpp
#pragma once

#include "../ishader.hpp"
#include "../../material/materialt.hpp"   // for IMaterial::As<T>()
#include "uniformcolorparams.hpp"

/**
 * @file uniformcolorshader.hpp
 * @brief Shader that returns a uniform color from its material parameters.
 */

/**
 * @class UniformColorShader
 * @brief Shades every surface with a single uniform color from UniformColorParams.
 *
 * Expects the bound material to be `MaterialT<UniformColorParams, UniformColorShader>`.
 */
class UniformColorShader final : public IShader {
public:
    /**
     * @brief Return the uniform color from the material’s parameter block.
     * @param surf Surface properties (unused).
     * @param m    Material bound at the call site.
     * @return The color stored in @ref UniformColorParams::color.
     */
    RGBColor Shade(const SurfaceProperties& /*surf*/, const IMaterial& m) const override {
        using MatBase = MaterialT<UniformColorParams, UniformColorShader>;
        const auto& p = m.As<MatBase>();
        return p.color;
    }
};
