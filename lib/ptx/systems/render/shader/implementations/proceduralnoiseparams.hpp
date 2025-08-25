// proceduralnoiseparams.hpp
#pragma once

#include <cstddef>
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

/**
 * @file proceduralnoiseparams.hpp
 * @brief Parameter block for procedural noise-driven gradient shading.
 *
 * @tparam N Number of colors in the gradient spectrum.
 */
template <size_t N>
struct ProceduralNoiseParamsT {
    RGBColor spectrum[N] = {};                 ///< Gradient spectrum (user-supplied).

    Vector3D noiseScale       { 0.01f, 0.01f, 0.01f };  ///< Noise frequency per axis.
    float    simplexDepth     = 0.0f;                   ///< Extra Z slice (time/phase).
    float    gradientPeriod   = 1.0f;                   ///< Repeat cycle in [0..1] units.
    float    hueShiftAngleDeg = 0.0f;                   ///< Hue rotation in degrees.
};
