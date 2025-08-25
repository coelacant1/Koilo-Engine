// uniformcolorparams.hpp
#pragma once
#include "../../../../core/color/rgbcolor.hpp"

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
};
