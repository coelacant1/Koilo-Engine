#pragma once
#include <array>

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"

/**
 * @file gradientparams.hpp
 * @brief Parameter block for linear/radial gradient configuration.
 *
 * Holds N color keys and transform/behavior controls used by gradient shaders/materials.
 * Pointers are non-owning unless otherwise noted.
 */

/**
 * @tparam N Number of colors in the gradient.
 * @struct GradientParams
 * @brief Editable gradient parameters and transform state.
 *
 * Members control the active color keys, optional base palette, spatial transforms
 * (offset/rotation), repetition, and rendering mode (linear/radial, stepped/smooth).
 */
template <size_t N>
struct GradientParams {
    /** Editable/active colors used by the gradient (size N). */
    std::array<RGBColor, N> colors{};     

    /**
     * Non-owning pointer to a base color array (size N expected).
     * Used by features like hue shifting or external palette updates.
     */
    const RGBColor*         baseColors{}; 

    /** XY translation applied to the gradient. */
    Vector2D positionOffset{};            

    /** Pivot (XY) about which @ref rotationAngle is applied. */
    Vector2D rotationOffset{};            

    /**
     * Period for repeating the gradient pattern.
     * Interpreted by the shader/material in its local units (default 1.0).
     */
    float    gradientPeriod{1.0f};        

    /** Rotation angle in degrees (around @ref rotationOffset). */
    float    rotationAngle{0.0f};         

    /**
     * Normalized shift of the gradient along its primary axis (typically X),
     * where implementation treats the range near [0..1].
     */
    float    gradientShift{0.0f};         

    /** Render as radial (true) or linear (false) gradient. */
    bool     isRadial{false};             

    /** Render with discrete steps (true) or smoothly interpolated (false). */
    bool     isStepped{false};            
};
