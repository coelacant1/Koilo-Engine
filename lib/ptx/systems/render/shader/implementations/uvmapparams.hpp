// uvmapparams.hpp
#pragma once

#include "../../../../assets/image/image.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file uvmapparams.hpp
 * @brief Parameters for UV mapping a texture.
 *
 * Holds the state required by UV-mapping shaders/materials to sample an @ref Image
 * using surface UVs, with optional U/V flips and a hue shift.
 */
struct UVMapParams {
    Image*   image   = nullptr;        ///< Non-owning pointer to the source image.
    Vector2D size    { 1.0f, 1.0f };   ///< Logical UV size (defaults to 1×1).
    bool     flipU   = true;           ///< If true, U is flipped: u' = 1 - u.
    bool     flipV   = false;          ///< If true, V is flipped.
    float    hueAngle = 0.0f;          ///< Hue shift in degrees applied to sampled color.

    PTX_BEGIN_FIELDS(UVMapParams)
        PTX_FIELD(UVMapParams, image, "Image", 0, 0),
        PTX_FIELD(UVMapParams, size, "Size", 0, 0),
        PTX_FIELD(UVMapParams, flipU, "Flip u", 0, 1),
        PTX_FIELD(UVMapParams, flipV, "Flip v", 0, 1),
        PTX_FIELD(UVMapParams, hueAngle, "Hue angle", __FLT_MIN__, __FLT_MAX__)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(UVMapParams)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(UVMapParams)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(UVMapParams)

};
