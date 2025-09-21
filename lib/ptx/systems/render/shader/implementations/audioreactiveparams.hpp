#pragma once

#include <cstddef>
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"

/**
 * @file audioreactiveparams.hpp
 * @brief Parameter block for audio-reactive gradient materials.
 *
 * @tparam N Number of gradient color keys.
 * @tparam B Number of spectrum bins.
 */

/**
 * @struct AudioReactiveParamsT
 * @brief Parameters for audio-reactive rendering and gradient control.
 *
 * Holds transform/appearance controls, behavior flags, external spectrum input,
 * optional bounce-processed data, and N gradient keys.
 *
 * - @ref samples points to B floats provided by the caller (non-owning).
 * - When bounce processing is active, @ref bounceData contains per-bin results.
 */
template <std::size_t N, std::size_t B>
struct AudioReactiveParamsT {
    // Transform / appearance
    Vector2D sizeHalf {96.0f, 48.0f};   ///< Half width/height (extent from center).
    Vector2D offset   {0.0f,  0.0f};    ///< Center position.
    float    angleDeg {0.0f};           ///< Rotation in degrees about @ref offset.
    float    hueDeg   {0.0f};           ///< Hue shift in degrees.

    // Circular ring mode
    bool     circular {false};          ///< Enable circular (ring) rendering.
    float    radius   {75.0f};          ///< Radius used when @ref circular is true.

    // Behavior flags
    bool     bounce   {false};          ///< Enable per-bin bounce processing.

    // Spectrum data (owned externally)
    const float* samples {nullptr};     ///< Pointer to B spectrum values (0..1 suggested).

    // Optional bounced data (written by material Update)
    float   bounceData[B] = {};         ///< Per-bin output when @ref bounce is true.

    // Gradient keys (unused entries remain zero if N > 6)
    RGBColor spectrum[N] = {
        RGBColor(255, 0,   0), RGBColor(255, 255,   0),
        RGBColor(  0, 255, 0), RGBColor(  0, 255, 255),
        RGBColor(  0, 0, 255), RGBColor(255,   0, 255)
    };

};
