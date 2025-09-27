// tvstaticparams.hpp
#pragma once

#include <cstddef>
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"

/**
 * @file tvstaticparams.hpp
 * @brief Parameter block for TV static shader.
 *
 * @tparam NNoise Number of gradient keys for noise coloration.
 * @tparam NScan  Number of gradient keys for scanline coloration.
 *
 * The shader reads only from this struct; a material is expected to update fields per frame.
 */
template <std::size_t NNoise, std::size_t NScan>
struct TVStaticParamsT {
    // --- Noise (simplex) controls ---
    RGBColor noiseSpectrum[NNoise] = {
        RGBColor(255,255,255),
        RGBColor(0,0,0),
        RGBColor(255,0,0),
        RGBColor(0,255,0),
        RGBColor(0,0,255)
    };                                  ///< Noise gradient keys (hue-shifted at runtime).
    float    noiseGradientPeriod {0.5f};///< Period for gradient repetition in noise space.
    float    noiseHueDeg         {0.0f};///< Hue shift (degrees) applied to noiseSpectrum.
    Vector3D noiseScale          {0.5f, 0.5f, 0.5f}; ///< Scale for sampling simplex noise (xyz).
    float    noiseZ              {0.0f};///< Temporal/depth coordinate for animated noise.

    // --- Scanline gradient controls ---
    RGBColor scanSpectrum[NScan] = {
        RGBColor(0,0,0), RGBColor(0,0,0), RGBColor(255,255,255), RGBColor(0,0,0),
        RGBColor(255,255,255), RGBColor(255,255,255), RGBColor(255,255,255),
        RGBColor(0,0,0), RGBColor(0,0,0), RGBColor(255,255,255)
    };                                  ///< Scanline gradient keys.
    float    scanGradientPeriod  {100.0f}; ///< Scanline gradient period.
    float    scanRotationDeg     {90.0f};  ///< Orientation (degrees); 90° = vertical stripes.
    float    scanShift           {0.0f};   ///< Phase shift for scanline animation.

    // --- Color bars overlay (replacement for ColorTest) ---
    Vector2D barsCenter          {96.0f, 72.0f}; ///< Bars region center.
    float    barsHueDeg          {0.0f};         ///< Hue shift (degrees) for bars.
    Vector2D barsSize            {192.0f, 96.0f};///< Bars region size for tiling.
    float    barsSoftness        {2.0f};         ///< Edge softness in pixels.

    // --- Blend constants ---
    float    scanMultiplyOpacity {0.75f};        ///< Multiply contribution opacity.

};
