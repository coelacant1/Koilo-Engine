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
        /* TODO: PTX_FIELD(NormalParams, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(NormalParams)
        /* TODO: PTX_METHOD_AUTO(NormalParams, Method, "Doc") */
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(NormalParams)
        /* TODO: PTX_CTOR0(NormalParams) or PTX_CTOR(NormalParams, ...) */
    PTX_END_DESCRIBE(NormalParams)
    
};
