#pragma once
#include "../../../../assets/image/imagesequence.hpp"

/**
 * @file imagesequenceparams.hpp
 * @brief Parameter block for materials that sample an ImageSequence.
 * @date 8/18/2025
 */

/**
 * @struct ImageSequenceParams
 * @brief Parameters controlling how a material samples and tints an image sequence.
 *
 * Fields:
 * - @ref sequence : Non-owning pointer to the sequence to sample. If nullptr, sampling is disabled.
 * - @ref hueAngle : Hue rotation in degrees applied to sampled colors.
 * - @ref useUV    : Sampling space selector:
 *                   - true  -> sample using surface UVs (e.g., sp.uvw).
 *                   - false -> sample using surface position XY.
 */
struct ImageSequenceParams {
    ImageSequence* sequence = nullptr;  ///< Non-owning; must outlive the material using it.
    float          hueAngle = 0.0f;     ///< Hue rotation in degrees (post-sample tint).
    bool           useUV    = true;     ///< true: use UVs; false: use position XY.
};
