#pragma once

#include <cstddef>
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file horizontalrainbowparams.hpp
 * @brief Parameter block for a horizontal rainbow gradient with N spectrum keys.
 *
 * Defines color spectrum keys, position offset for scrolling, rotation (in degrees),
 * periodicity of the gradient in world units, and per-update scroll speed.
 *
 * @tparam N Number of spectrum keys.
 */

/**
 * @struct HorizontalRainbowParamsT
 * @brief Parameters used by a horizontal rainbow gradient shader/material.
 *
 * Defaults:
 * - spectrum: classic 6-color rainbow (extra entries, if any, remain {0,0,0})
 * - rotationDeg: 90° -> horizontal bands
 * - gradientPeriod: repeat distance in world units
 * - scrollSpeed: delta applied to the X axis each Update()
 */
template <std::size_t N>
struct HorizontalRainbowParamsT {
    /// Color keys for the gradient. For N > 6, remaining entries default to {0,0,0}.
    RGBColor spectrum[N] = {
        RGBColor(255,   0,   0), RGBColor(255, 255,   0),
        RGBColor(  0, 255,   0), RGBColor(  0, 255, 255),
        RGBColor(  0,   0, 255), RGBColor(255,   0, 255)
    };

    Vector2D positionOffset {0.0f, 0.0f}; ///< Animated scroll offset (world units).
    float    rotationDeg    = 90.0f;      ///< Rotation in degrees (90° -> horizontal gradient).
    float    gradientPeriod = 96.0f;      ///< Repeat distance in world units.
    float    scrollSpeed    = -1.0f;      ///< Units per Update() applied along +X.

    /* NOTE: HorizontalRainbowParamsT is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(HorizontalRainbowParamsT)
        PTX_FIELD(HorizontalRainbowParamsT, spectrum, "Spectrum", 0, 0),
        PTX_FIELD(HorizontalRainbowParamsT, positionOffset, "Position offset", 0, 0),
        PTX_FIELD(HorizontalRainbowParamsT, rotationDeg, "Rotation deg", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(HorizontalRainbowParamsT, gradientPeriod, "Gradient period", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(HorizontalRainbowParamsT, scrollSpeed, "Scroll speed", __FLT_MIN__, __FLT_MAX__)
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(HorizontalRainbowParamsT)
        /* TODO: PTX_METHOD_AUTO(HorizontalRainbowParamsT, Method, "Doc") */
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(HorizontalRainbowParamsT)
        /* TODO: PTX_CTOR0(HorizontalRainbowParamsT) or PTX_CTOR(HorizontalRainbowParamsT, ...) */
    PTX_END_DESCRIBE(HorizontalRainbowParamsT)
    
};
