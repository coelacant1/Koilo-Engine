// uniformcolorparams.hpp
#pragma once
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file uniformcolorparams.hpp
 * @brief Parameter block for a uniform color material.
 */

/**
 * @struct UniformColorParams
 * @brief Holds the active output color and a base reference color.
 *
 * - @ref color is the output used by the shader.
 * - @ref base can be used by effects that derive hue/offsets.
 */
struct UniformColorParams {
    RGBColor color{0,0,0};  ///< Active output color
    RGBColor base{0,0,0};   ///< Reference color (e.g., for hue shift)

    PTX_BEGIN_FIELDS(UniformColorParams)
        PTX_FIELD(UniformColorParams, color, "Color", 0, 0),
        PTX_FIELD(UniformColorParams, base, "Base", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(UniformColorParams)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(UniformColorParams)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(UniformColorParams)

};
