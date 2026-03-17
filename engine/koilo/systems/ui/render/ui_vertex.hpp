// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_vertex.hpp
 * @brief Shared UI vertex format for all GPU renderers.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */

#pragma once

#include "../../../registry/reflect_macros.hpp"
#include <cstddef>
#include <cstdint>

namespace koilo {
namespace ui {

/** @struct UIVertex @brief Vertex for batched UI rendering (52 bytes). */
struct UIVertex {
    float x, y;         ///< screen-space position
    float u, v;         ///< texture coordinates / local normalized pos
    uint8_t r, g, b, a; ///< vertex color (RGBA8)
    float sdf[4];       ///< SDF params (halfW, halfH, borderWidth, 0)
    float radii[4];     ///< per-corner radius (TL, TR, BR, BL)

    KL_BEGIN_FIELDS(UIVertex)
        KL_FIELD(UIVertex, y, "Y", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(UIVertex)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(UIVertex)
        /* No reflected ctors. */
    KL_END_DESCRIBE(UIVertex)

};
static_assert(sizeof(UIVertex) == 52, "UIVertex should be 52 bytes");

} // namespace ui
} // namespace koilo
