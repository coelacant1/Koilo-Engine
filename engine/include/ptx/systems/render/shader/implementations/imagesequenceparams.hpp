#pragma once
#include "../../../../assets/image/imagesequence.hpp"
#include "../../../../registry/reflect_macros.hpp"

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

    PTX_BEGIN_FIELDS(ImageSequenceParams)
        PTX_FIELD(ImageSequenceParams, sequence, "Sequence", 0, 0),
        PTX_FIELD(ImageSequenceParams, hueAngle, "Hue angle", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(ImageSequenceParams, useUV, "Use uv", 0, 1)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(ImageSequenceParams)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(ImageSequenceParams)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(ImageSequenceParams)

};
