// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file drag_drop.hpp
 * @brief General-purpose drag-and-drop framework for the Koilo UI system.
 *
 * Provides DragPayload for cross-widget data transfer, type tags for
 * payload identification, and the DnD state machine driven by UIContext.
 *
 * @date 03/12/2026
 * @author Coela Can't
 */

#pragma once

#include <cstdint>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

/// Well-known drag payload type tags.
namespace DragType {
    static constexpr uint32_t None      = 0;
    static constexpr uint32_t Widget    = 1;   ///< Reparenting a widget/tree node
    static constexpr uint32_t Asset     = 2;   ///< File/asset reference
    static constexpr uint32_t Color     = 3;   ///< Color4 value
    static constexpr uint32_t Custom    = 0x100; ///< User-defined types start here
}

/** @class DragPayload @brief Payload carried during a drag operation. */
struct DragPayload {
    uint32_t    typeTag       = DragType::None; ///< What kind of data is being dragged
    const void* data          = nullptr;        ///< Opaque pointer to payload data
    int         sourceWidget  = -1;             ///< Pool index of the widget that initiated the drag
    uint32_t    labelId       = 0;              ///< StringId for ghost label text
    uint32_t    iconTextureId = 0;              ///< Optional icon texture for ghost overlay

    /** @brief Check whether a drag operation is in progress. */
    bool IsActive() const { return typeTag != DragType::None; }
    /** @brief Reset payload to default state. */
    void Clear() { typeTag = DragType::None; data = nullptr; sourceWidget = -1; labelId = 0; iconTextureId = 0; }

    KL_BEGIN_FIELDS(DragPayload)
        KL_FIELD(DragPayload, typeTag, "Type tag", 0, 4294967295)
    KL_END_FIELDS

    KL_BEGIN_METHODS(DragPayload)
        KL_METHOD_AUTO(DragPayload, Clear, "Clear")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(DragPayload)
        /* No reflected ctors. */
    KL_END_DESCRIBE(DragPayload)

};

/// Result of a drop target query - where to insert relative to the target.
enum class DropPosition : uint8_t {
    Into   = 0, ///< Drop as child of target
    Before = 1, ///< Insert before target in parent's child list
    After  = 2  ///< Insert after target in parent's child list
};

} // namespace ui
} // namespace koilo
