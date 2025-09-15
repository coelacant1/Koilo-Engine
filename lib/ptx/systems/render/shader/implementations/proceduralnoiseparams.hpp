// proceduralnoiseparams.hpp
#pragma once

#include <cstddef>
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../registry/reflect_macros.hpp"

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

    /* NOTE: ProceduralNoiseParamsT is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(ProceduralNoiseParamsT)
        PTX_FIELD(ProceduralNoiseParamsT, spectrum, "Spectrum", 0, 0),
        PTX_FIELD(ProceduralNoiseParamsT, noiseScale, "Noise scale", 0, 0),
        PTX_FIELD(ProceduralNoiseParamsT, simplexDepth, "Simplex depth", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(ProceduralNoiseParamsT, gradientPeriod, "Gradient period", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(ProceduralNoiseParamsT, hueShiftAngleDeg, "Hue shift angle deg", __FLT_MIN__, __FLT_MAX__)
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(ProceduralNoiseParamsT)
        /* TODO: PTX_METHOD_AUTO(ProceduralNoiseParamsT, Method, "Doc") */
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(ProceduralNoiseParamsT)
        /* TODO: PTX_CTOR0(ProceduralNoiseParamsT) or PTX_CTOR(ProceduralNoiseParamsT, ...) */
    PTX_END_DESCRIBE(ProceduralNoiseParamsT)
    
};
