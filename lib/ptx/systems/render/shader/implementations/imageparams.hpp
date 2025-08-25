#pragma once

#include "../../../../assets/image/image.hpp"

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
};
