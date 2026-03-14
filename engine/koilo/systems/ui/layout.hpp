// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file layout.hpp
 * @brief Flexbox-inspired layout engine for the UI widget tree.
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include "widget.hpp"
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

/**
 * @class LayoutEngine
 * @brief Computes the layout of a widget subtree using flexbox and CSS Grid algorithms.
 */
class LayoutEngine {
public:
    /**
     * @brief Run layout from the root. Sets computedRect on all widgets.
     * @param pool      Widget pool containing all widgets.
     * @param rootIndex Index of the root widget in the pool.
     */
    void Compute(WidgetPool& pool, int rootIndex) {
        Widget* root = pool.Get(rootIndex);
        if (!root || !root->IsLayoutVisible()) return;
        // Root's computedRect is set by caller (UIContext); don't overwrite it.
        if (root->childCount > 0) {
            LayoutChildren(pool, *root);
        }
        root->flags.dirty = 0;
    }

private:
    /** @brief Recursively offset the computed rect of a widget and all its descendants. */
    void OffsetSubtree(WidgetPool& pool, int index, float dx, float dy) {
        Widget* w = pool.Get(index);
        if (!w) return;
        w->computedRect.x += dx;
        w->computedRect.y += dy;
        for (int i = 0; i < w->childCount; ++i)
            OffsetSubtree(pool, w->children[i], dx, dy);
    }

    /** @brief Lay out a single widget within the given parent rect. */
    void LayoutWidget(WidgetPool& pool, int index, const Rect& parentRect);
    /** @brief Compute a widget's rect from its size mode and parent rect. */
    void ComputeRect(Widget& w, const Rect& parentRect);

    /** @brief Lay out all children of a parent (flexbox or grid dispatch). */
    void LayoutChildren(WidgetPool& pool, Widget& parent);

    /** @brief CSS Grid layout algorithm. */
    void LayoutGrid(WidgetPool& pool, Widget& parent);

    KL_BEGIN_FIELDS(LayoutEngine)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(LayoutEngine)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(LayoutEngine)
        /* No reflected ctors. */
    KL_END_DESCRIBE(LayoutEngine)

};

} // namespace ui
} // namespace koilo
