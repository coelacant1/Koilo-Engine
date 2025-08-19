// oscilloscopeparams.hpp
#pragma once

#include <cstddef>
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"

/**
 * @file oscilloscopeparams.hpp
 * @brief Parameter block for an audio oscilloscope material/shader.
 *
 * @tparam N Number of gradient colors.
 * @tparam B Number of sample bins (FFT or time-domain).
 *
 * @details Size is stored as half-extents (width/2, height/2).
 */
template <size_t N, size_t B>
struct OscilloscopeParamsT {
    // --- Appearance / transform ---
    Vector2D sizeHalf {80.0f, 40.0f};  ///< Half-width and half-height.
    Vector2D offset   {0.0f,  0.0f};   ///< Center position in world space.
    float    angleDeg {0.0f};          ///< Rotation around @ref offset (degrees).
    float    hueDeg   {0.0f};          ///< Hue shift applied to final gradient color (degrees).

    // --- Gradient spectrum ---
    /**
     * @brief N-key gradient colors (defaults to a 6-color rainbow if N>=6).
     * Extra entries (when N>6) remain {0,0,0}.
     */
    RGBColor spectrum[N] = {
        RGBColor(255, 0,   0  ), RGBColor(255, 255, 0  ),
        RGBColor(0,   255, 0  ), RGBColor(0,   255, 255),
        RGBColor(0,   0,   255), RGBColor(255, 0,   255)
    };

    // --- Signal ---
    static constexpr size_t kBins = B;  ///< Compile-time number of sample bins.
    const float* samples = nullptr;     ///< Non-owning pointer to B floats.

    // --- Dynamic scaling (set by the material wrapper) ---
    float minValue = 0.0f;              ///< Minimum input value mapped to 0.
    float maxValue = 1.0f;              ///< Maximum input value mapped to 1.
    float midPoint = 0.5f;              ///< Optional midpoint reference in [0,1].

    // --- Rendering controls ---
    float lineThickness = 0.1f;         ///< Line thickness as fraction of sizeHalf.Y.
    float heightClamp   = 0.75f;        ///< Vertical amplitude clamp (0..1 of sizeHalf.Y).
};
