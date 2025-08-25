#pragma once
#include "../ishader.hpp"  // IMaterial fwd-usage
#include "../../../../core/geometry/2d/shape.hpp"

/**
 * @file materialmaskparams.hpp
 * @brief Parameter block for a mask material that blends two materials using a 2D shape as the mask.
 *
 * The mask controls which material contributes to the final color at a surface point.
 * Opacity scales the mask’s effect in [0..1].
 */

/**
 * @struct MaterialMaskParams
 * @brief Parameters for masking between an inner (shape) and outer material.
 *
 * The @ref shape defines the masked region. @ref materialShape shades inside the region,
 * while @ref materialOuter shades outside. @ref opacity modulates the mask strength.
 */
struct MaterialMaskParams {
    const IMaterial* materialShape = nullptr;  ///< Material used inside the mask region (non-owning).
    const IMaterial* materialOuter = nullptr;  ///< Material used outside the mask region (non-owning).
    Shape*           shape         = nullptr;  ///< Mask geometry in world space (non-owning).
    float            opacity       = 1.0f;     ///< Global mask opacity in [0..1].
};
