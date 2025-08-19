// uvmapparams.hpp
#pragma once

#include "../../../../assets/image/image.hpp"
#include "../../../../core/math/vector2d.hpp"

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
};
