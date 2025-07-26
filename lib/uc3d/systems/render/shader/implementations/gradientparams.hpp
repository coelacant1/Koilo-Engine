#pragma once

#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

struct GradientParams {
    uint8_t colorCount;
    RGBColor* rgbColors; ///< Array of colors used in the gradient.
    RGBColor* baseRGBColors; ///< Pointer to the base RGB colors.
    Vector2D positionOffset; ///< Offset for the gradient's position.
    Vector2D rotationOffset; ///< Point about which the gradient rotates.
    float gradientPeriod = 1.0f; ///< Period of the gradient repetition.
    float rotationAngle = 0.0f; ///< Angle for rotating the gradient.
    bool isRadial = false; ///< Whether the gradient is radial.
    bool isStepped = false; ///< Whether the gradient is stepped.
    float gradientShift = 0.0f; ///< Shift in the gradient pattern.
};
