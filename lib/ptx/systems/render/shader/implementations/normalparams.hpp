// normalparams.hpp
#pragma once

#include "../../../../registry/reflect_macros.hpp"
/**
 * @file normalparams.hpp
 * @brief Parameter block for classic normal visualization (currently empty).
 *
 * This struct is kept to align with the MaterialT pattern and allow future tuning
 * (e.g., normalization flags, scaling, bias) without changing the shader API.
 */
struct NormalParams {
    // intentionally empty

    PTX_BEGIN_FIELDS(NormalParams)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(NormalParams)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(NormalParams)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(NormalParams)

};
