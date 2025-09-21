#pragma once

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"
#include "../../../../registry/reflect_macros.hpp"

#include "../../shader/implementations/uniformcolorparams.hpp"
#include "../../shader/implementations/uniformcolorshader.hpp"

/**
 * @file uniformcolormaterial.hpp
 * @brief Solid/flat color material (legacy SimpleMaterial equivalent).
 *
 * Preserves ergonomic helpers for setting a base color and applying hue shifts.
 */

/**
 * @class UniformColorMaterial
 * @brief Material that shades with a uniform RGB color.
 *
 * Wraps @ref UniformColorParams and @ref UniformColorShader via @ref MaterialT.
 * Provides helpers to set a base color and derive a shifted hue.
 */
class UniformColorMaterial : public MaterialT<UniformColorParams, UniformColorShader> {
    using Base = MaterialT<UniformColorParams, UniformColorShader>;
public:
    using Base::Base; ///< Inherit param-block forwarding constructor

    /**
     * @brief Construct with an initial RGB color.
     * @param rgb Initial color; stored in both @c color and @c base.
     */
    explicit UniformColorMaterial(RGBColor rgb) {
        this->color = rgb;
        this->base  = rgb;
    }

    /**
     * @brief Apply a hue shift relative to the current base color.
     * @param hueDeg Hue rotation in degrees.
     *
     * Updates @c color by rotating the hue of @c base; @c base is unchanged.
     */
    void HueShift(float hueDeg) {
        this->color = this->base.HueShift(hueDeg);
    }

    /**
     * @brief Set the current RGB color and update the base reference color.
     * @param rgb New color value applied to both @c color and @c base.
     */
    void SetRGB(RGBColor rgb) {
        this->color = rgb;
        this->base  = rgb;
    }

    PTX_BEGIN_FIELDS(UniformColorMaterial)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(UniformColorMaterial)
        PTX_METHOD_AUTO(UniformColorMaterial, HueShift, "Hue shift"),
        PTX_METHOD_AUTO(UniformColorMaterial, SetRGB, "Set rgb")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(UniformColorMaterial)
        PTX_CTOR(UniformColorMaterial, RGBColor)
    PTX_END_DESCRIBE(UniformColorMaterial)

};
