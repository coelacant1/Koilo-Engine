#pragma once

#include "../ishader.hpp"  // for IMaterial*
#include <cstdint>
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file
 * @brief Parameters for depth-to-gradient mapping in a shader/material pair.
 *
 * Encodes how a depth value is derived from a surface position by selecting an axis,
 * applying an offset, and mapping a span (@ref DepthParams::depth) into the [0..1]
 * range used to sample a gradient material.
 */

/**
 * @enum DepthAxis
 * @brief Axis selection for depth mapping.
 * @var DepthAxis::X Use the X component.
 * @var DepthAxis::Y Use the Y component.
 * @var DepthAxis::Z Use the Z component.
 */

enum class DepthAxis : uint8_t { X, Y, Z };

/**
 * @struct DepthParams
 * @brief Parameter block for depth-based gradient sampling.
 *
 * The mapped value is computed along @ref axis, shifted by @ref offset, and scaled by
 * @ref depth so that the resulting normalized value lies in [0..1] for sampling
 * the @ref gradient material.
 */
struct DepthParams {

    DepthAxis        axis   { DepthAxis::Z };   ///< Axis used to compute depth.

    float            depth  { 1.0f };           ///< Span length mapped into [0..1].

    float            offset { 0.0f };           ///< Offset applied along the axis.

    const IMaterial* gradient { nullptr };      ///< Non-owning gradient material (expects input in [0..1]).

    PTX_BEGIN_FIELDS(DepthParams)
        PTX_FIELD(DepthParams, axis, "Axis", 0, 0),
        PTX_FIELD(DepthParams, depth, "Depth", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(DepthParams, offset, "Offset", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(DepthParams, gradient, "Gradient", 0, 0)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(DepthParams)
        /* No reflected methods. */
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(DepthParams)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(DepthParams)

};
