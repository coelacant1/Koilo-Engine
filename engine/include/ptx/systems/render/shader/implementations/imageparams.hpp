#pragma once

#include "../../../../assets/image/image.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file imageparams.hpp
 * @brief Parameters for image-based sampling in materials/shaders.
 *
 * Holds a non-owning image pointer and sampling controls:
 * - hueAngle: extra hue rotation in degrees applied to sampled color.
 * - useUV: choose between surface UVs or XY position for sampling coordinates.
 */
struct ImageParams {
    Image* image    = nullptr; ///< Non-owning pointer to the source Image; must remain valid.
    float  hueAngle = 0.0f;    ///< Hue rotation in degrees applied after sampling.
    bool   useUV    = true;    ///< true: sample surface UV; false: sample surface XY position.

    PTX_BEGIN_FIELDS(ImageParams)
        PTX_FIELD(ImageParams, image, "Image", 0, 0),
        PTX_FIELD(ImageParams, hueAngle, "Hue angle", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(ImageParams, useUV, "Use uv", 0, 1)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(ImageParams)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(ImageParams)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(ImageParams)

};
