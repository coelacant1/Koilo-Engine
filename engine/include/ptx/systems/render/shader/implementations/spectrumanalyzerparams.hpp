// spectrumanalyzerparams.hpp
#pragma once

#include <cstddef>
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"

/**
 * @file spectrumanalyzerparams.hpp
 * @brief Parameter block for the spectrum analyzer material/shader.
 *
 * @tparam N Gradient key count.
 * @tparam B Number of spectrum bins.
 */

/**
 * @brief Parameter set for spectrum analyzer rendering.
 * @tparam N Gradient key count.
 * @tparam B Number of spectrum bins.
 */
template <std::size_t N, std::size_t B>
struct SpectrumAnalyzerParamsT {
    // Transform / appearance
    Vector2D sizeHalf {96.0f, 48.0f};    ///< Half width/height (extents from center).
    Vector2D offset   {0.0f,  0.0f};     ///< Center position.
    float    angleDeg {0.0f};            ///< Rotation about @ref offset in degrees.
    float    hueDeg   {0.0f};            ///< Hue shift applied to the gradient (degrees).

    // Y-shaping & options
    bool     mirrorY     {false};        ///< Mirror around X axis.
    bool     flipY       {false};        ///< Invert Y ramp.
    bool     bounce      {false};        ///< Use BouncePhysics per bin.
    float    heightScale {3.0f};         ///< Additional gain applied to bar height.

    // Data
    const float* samples {nullptr};      ///< External pointer to @p B floats (non-owning).
    float        bounceData[B] = {};     ///< Filled when @ref bounce is true (or mirrored from samples).

    // Gradient keys (defaults to a 6-key rainbow; remaining entries stay {0,0,0} if N>6)
    RGBColor spectrum[N] = {
        RGBColor(255, 0,   0), RGBColor(255, 255,   0),
        RGBColor(  0, 255, 0), RGBColor(  0, 255, 255),
        RGBColor(  0, 0, 255), RGBColor(255,   0, 255)
    };

};
