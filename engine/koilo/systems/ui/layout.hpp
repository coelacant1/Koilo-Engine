// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file layout.hpp
 * @brief Flexbox-inspired layout engine for the UI widget tree.
 *
 * Layout is computed top-down from a root widget. Only widgets with
 * dirty flags are re-laid-out. Produces computed rects for rendering.
 *
 * @date 03/08/2026
 * @author Coela
 */

#pragma once

#include "widget.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

/// Compute the layout of a widget subtree.
/// The root widget's computedRect must already be set (typically to the screen rect).
class LayoutEngine {
public:
    /// Run layout from the root. Sets computedRect on all widgets.
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
    /// Recursively offset the computed rect of a widget and all its descendants.
    void OffsetSubtree(WidgetPool& pool, int index, float dx, float dy) {
        Widget* w = pool.Get(index);
        if (!w) return;
        w->computedRect.x += dx;
        w->computedRect.y += dy;
        for (int i = 0; i < w->childCount; ++i)
            OffsetSubtree(pool, w->children[i], dx, dy);
    }

    void LayoutWidget(WidgetPool& pool, int index, const Rect& parentRect) {
        Widget* w = pool.Get(index);
        if (!w || !w->IsLayoutVisible()) return;

        // Compute this widget's rect based on its size mode and parent
        ComputeRect(*w, parentRect);

        // Apply relative offset after normal positioning
        if (w->positionMode == PositionMode::Relative) {
            w->computedRect.x += w->posLeft - w->posRight;
            w->computedRect.y += w->posTop - w->posBottom;
        }

        // If this widget has children, lay them out
        if (w->childCount > 0) {
            LayoutChildren(pool, *w);
        }

        w->flags.dirty = 0;
    }

    void ComputeRect(Widget& w, const Rect& parentRect) {
        float x = parentRect.x;
        float y = parentRect.y;
        float pw = parentRect.w;
        float ph = parentRect.h;

        // Apply anchors if non-zero
        bool hasAnchors = (w.anchors.minX != 0.0f || w.anchors.minY != 0.0f ||
                           w.anchors.maxX != 0.0f || w.anchors.maxY != 0.0f);

        if (hasAnchors) {
            float ax0 = x + pw * w.anchors.minX;
            float ay0 = y + ph * w.anchors.minY;
            float ax1 = x + pw * w.anchors.maxX;
            float ay1 = y + ph * w.anchors.maxY;
            w.computedRect.x = ax0 + w.localX + w.margin.left;
            w.computedRect.y = ay0 + w.localY + w.margin.top;
            w.computedRect.w = (ax1 - ax0) - w.margin.left - w.margin.right;
            w.computedRect.h = (ay1 - ay0) - w.margin.top - w.margin.bottom;
        } else {
            // Width
            switch (w.widthMode) {
                case SizeMode::Fixed:
                case SizeMode::Auto:
                    w.computedRect.w = w.localW;
                    break;
                case SizeMode::Percent:
                    w.computedRect.w = pw * (w.localW / 100.0f);
                    break;
                case SizeMode::FillRemaining:
                    w.computedRect.w = pw - w.margin.left - w.margin.right;
                    break;
                case SizeMode::FitContent:
                case SizeMode::MinContent:
                case SizeMode::MaxContent:
                    w.computedRect.w = w.localW; // refined after children layout
                    break;
            }

            // Height
            switch (w.heightMode) {
                case SizeMode::Fixed:
                case SizeMode::Auto:
                    w.computedRect.h = w.localH;
                    break;
                case SizeMode::Percent:
                    w.computedRect.h = ph * (w.localH / 100.0f);
                    break;
                case SizeMode::FillRemaining:
                    w.computedRect.h = ph - w.margin.top - w.margin.bottom;
                    break;
                case SizeMode::FitContent:
                case SizeMode::MinContent:
                case SizeMode::MaxContent:
                    w.computedRect.h = w.localH; // refined after children layout
                    break;
            }

            w.computedRect.x = x + w.localX + w.margin.left;
            w.computedRect.y = y + w.localY + w.margin.top;
        }

        // Apply min/max constraints
        if (w.minWidth > 0.0f && w.computedRect.w < w.minWidth)
            w.computedRect.w = w.minWidth;
        if (w.maxWidth > 0.0f && w.computedRect.w > w.maxWidth)
            w.computedRect.w = w.maxWidth;
        if (w.minHeight > 0.0f && w.computedRect.h < w.minHeight)
            w.computedRect.h = w.minHeight;
        if (w.maxHeight > 0.0f && w.computedRect.h > w.maxHeight)
            w.computedRect.h = w.maxHeight;

        // Aspect ratio: derive missing dimension from the set one
        if (w.aspectRatio > 0.0f) {
            bool wAuto = (w.widthMode == SizeMode::Auto || w.widthMode == SizeMode::FitContent);
            bool hAuto = (w.heightMode == SizeMode::Auto || w.heightMode == SizeMode::FitContent);
            if (wAuto && !hAuto)
                w.computedRect.w = w.computedRect.h * w.aspectRatio;
            else if (!wAuto)
                w.computedRect.h = w.computedRect.w / w.aspectRatio;
        }

        // box-sizing: content-box - expand outer box to include padding
        // (default border-box: specified size already includes padding)
        if (w.boxSizing == BoxSizing::ContentBox) {
            w.computedRect.w += w.padding.left + w.padding.right;
            w.computedRect.h += w.padding.top + w.padding.bottom;
        }
    }

    void LayoutChildren(WidgetPool& pool, Widget& parent) {
        // Grid layout branch
        if (parent.layout.isGrid) {
            LayoutGrid(pool, parent);
            return;
        }

        Rect contentRect = {
            parent.computedRect.x + parent.padding.left,
            parent.computedRect.y + parent.padding.top,
            parent.computedRect.w - parent.padding.left - parent.padding.right,
            parent.computedRect.h - parent.padding.top - parent.padding.bottom
        };

        bool isRow = (parent.layout.direction == LayoutDirection::Row);
        float totalAvailable = isRow ? contentRect.w : contentRect.h;

        // Pass 1: measure fixed/percent children, collect flex info (skip absolute)
        float usedSpace = 0.0f;
        float totalGrow = 0.0f;
        float totalShrink = 0.0f;
        int fillCount = 0;
        int visibleCount = 0;

        for (int i = 0; i < parent.childCount; ++i) {
            Widget* child = pool.Get(parent.children[i]);
            if (!child || !child->IsLayoutVisible()) continue;
            if (child->positionMode == PositionMode::Absolute) continue;
            visibleCount++;

            SizeMode mode = isRow ? child->widthMode : child->heightMode;
            float basis = 0.0f;

            if (mode == SizeMode::FillRemaining) {
                // Use flexBasis if set, otherwise 0
                basis = child->flexBasis;
                fillCount++;
                totalGrow += (child->flexGrow > 0.0f) ? child->flexGrow : 1.0f;
                totalShrink += child->flexShrink;
            } else {
                float parentSize = isRow ? contentRect.w : contentRect.h;
                switch (mode) {
                    case SizeMode::Fixed:
                    case SizeMode::Auto:
                        basis = isRow ? child->localW : child->localH;
                        break;
                    case SizeMode::Percent:
                        basis = parentSize * ((isRow ? child->localW : child->localH) / 100.0f);
                        break;
                    case SizeMode::FitContent:
                    case SizeMode::MinContent:
                    case SizeMode::MaxContent:
                        basis = isRow ? child->localW : child->localH;
                        break;
                    default:
                        break;
                }
                // Non-fill children with flexGrow still participate
                if (child->flexGrow > 0.0f && mode != SizeMode::FillRemaining) {
                    totalGrow += child->flexGrow;
                }
            }
            float margins = isRow ? (child->margin.left + child->margin.right)
                                  : (child->margin.top + child->margin.bottom);
            usedSpace += basis + margins;
        }

        // Account for spacing between visible children
        // When space-between/around/evenly is active, spacing is computed from remaining space instead
        bool hasSpaceAlign = (parent.layout.mainAlign == Alignment::SpaceBetween ||
                              parent.layout.mainAlign == Alignment::SpaceAround ||
                              parent.layout.mainAlign == Alignment::SpaceEvenly);
        if (!hasSpaceAlign && visibleCount > 1)
            usedSpace += parent.layout.spacing * (visibleCount - 1);

        float remainingSpace = totalAvailable - usedSpace;

        // Flex-shrink: when children exceed available space, shrink proportionally
        if (remainingSpace < 0.0f && totalShrink > 0.0f) {
            float deficit = -remainingSpace;
            for (int i = 0; i < parent.childCount; ++i) {
                Widget* child = pool.Get(parent.children[i]);
                if (!child || !child->IsLayoutVisible()) continue;
                if (child->positionMode == PositionMode::Absolute) continue;
                if (child->flexShrink <= 0.0f) continue;

                SizeMode mode = isRow ? child->widthMode : child->heightMode;
                if (mode == SizeMode::FillRemaining) continue; // flex-grow items handle this

                float shrinkAmount = deficit * (child->flexShrink / totalShrink);
                float minSize = isRow ? child->minWidth : child->minHeight;
                if (isRow) {
                    child->localW = std::max(minSize, child->localW - shrinkAmount);
                } else {
                    child->localH = std::max(minSize, child->localH - shrinkAmount);
                }
            }
            // Recompute remaining space after shrinking
            usedSpace = 0.0f;
            for (int i = 0; i < parent.childCount; ++i) {
                Widget* child = pool.Get(parent.children[i]);
                if (!child || !child->IsLayoutVisible()) continue;
                if (child->positionMode == PositionMode::Absolute) continue;
                SizeMode mode = isRow ? child->widthMode : child->heightMode;
                float basis = (mode == SizeMode::FillRemaining) ? child->flexBasis
                    : (isRow ? child->localW : child->localH);
                float margins = isRow ? (child->margin.left + child->margin.right)
                                      : (child->margin.top + child->margin.bottom);
                usedSpace += basis + margins;
            }
            if (!hasSpaceAlign && visibleCount > 1)
                usedSpace += parent.layout.spacing * (visibleCount - 1);
            remainingSpace = totalAvailable - usedSpace;
        }

        // Pass 2: distribute remaining space and position children
        float cursor = 0.0f;

        // Compute spacing for space-between/around/evenly
        float extraGap = 0.0f;
        float startOffset = 0.0f;
        if (visibleCount > 0) {
            if (parent.layout.mainAlign == Alignment::SpaceBetween && visibleCount > 1) {
                extraGap = std::max(0.0f, remainingSpace / (visibleCount - 1));
            } else if (parent.layout.mainAlign == Alignment::SpaceAround && visibleCount > 0) {
                extraGap = std::max(0.0f, remainingSpace / visibleCount);
                startOffset = extraGap * 0.5f;
            } else if (parent.layout.mainAlign == Alignment::SpaceEvenly && visibleCount > 0) {
                extraGap = std::max(0.0f, remainingSpace / (visibleCount + 1));
                startOffset = extraGap;
            }
        }
        cursor = startOffset;

        for (int i = 0; i < parent.childCount; ++i) {
            Widget* child = pool.Get(parent.children[i]);
            if (!child || !child->IsLayoutVisible()) continue;
            if (child->positionMode == PositionMode::Absolute) continue;

            SizeMode mainMode = isRow ? child->widthMode : child->heightMode;
            SizeMode savedMode = mainMode;
            SizeMode savedCross = isRow ? child->heightMode : child->widthMode;

            if (mainMode == SizeMode::FillRemaining) {
                // Proportional flex-grow distribution
                float growFactor = (child->flexGrow > 0.0f) ? child->flexGrow : 1.0f;
                float share = (totalGrow > 0.0f)
                    ? child->flexBasis + (std::max(0.0f, remainingSpace) * growFactor / totalGrow)
                    : child->flexBasis;
                float margins = isRow ? (child->margin.left + child->margin.right)
                                      : (child->margin.top + child->margin.bottom);
                if (isRow) {
                    child->localW = std::max(0.0f, share - margins);
                    child->widthMode = SizeMode::Fixed;
                } else {
                    child->localH = std::max(0.0f, share - margins);
                    child->heightMode = SizeMode::Fixed;
                }
            }

            // Set child position along main axis
            if (isRow) {
                child->localX = cursor;
                child->localY = 0.0f;
            } else {
                child->localX = 0.0f;
                child->localY = cursor;
            }

            // Apply cross-axis stretch (Auto mode allows stretch; Fixed/Percent don't)
            if (isRow && parent.layout.crossAlign == Alignment::Stretch) {
                if (child->heightMode == SizeMode::Auto || child->heightMode == SizeMode::FitContent)
                    child->heightMode = SizeMode::FillRemaining;
            } else if (!isRow && parent.layout.crossAlign == Alignment::Stretch) {
                if (child->widthMode == SizeMode::Auto || child->widthMode == SizeMode::FitContent)
                    child->widthMode = SizeMode::FillRemaining;
            }

            LayoutWidget(pool, parent.children[i], contentRect);

            // margin: auto - center along the axis with auto margins
            if (isRow) {
                if (child->marginAuto.top && child->marginAuto.bottom) {
                    float extra = contentRect.h - child->computedRect.h;
                    child->computedRect.y = contentRect.y + extra * 0.5f;
                } else if (child->marginAuto.top) {
                    child->computedRect.y = contentRect.y + contentRect.h - child->computedRect.h;
                } else if (child->marginAuto.bottom) {
                    child->computedRect.y = contentRect.y;
                }
            } else {
                if (child->marginAuto.left && child->marginAuto.right) {
                    float extra = contentRect.w - child->computedRect.w;
                    child->computedRect.x = contentRect.x + extra * 0.5f;
                } else if (child->marginAuto.left) {
                    child->computedRect.x = contentRect.x + contentRect.w - child->computedRect.w;
                } else if (child->marginAuto.right) {
                    child->computedRect.x = contentRect.x;
                }
            }

            // Restore original size modes (layout may have changed them temporarily)
            if (isRow) {
                child->widthMode = savedMode;
                child->heightMode = savedCross;
            } else {
                child->heightMode = savedMode;
                child->widthMode = savedCross;
            }

            // Advance cursor
            float childMainSize = isRow ? child->computedRect.w : child->computedRect.h;
            float margins = isRow ? (child->margin.left + child->margin.right)
                                  : (child->margin.top + child->margin.bottom);

            bool hasSpaceAlign = (parent.layout.mainAlign == Alignment::SpaceBetween ||
                                  parent.layout.mainAlign == Alignment::SpaceAround ||
                                  parent.layout.mainAlign == Alignment::SpaceEvenly);
            // When space-* alignment is active, use extraGap instead of fixed spacing
            float gap = hasSpaceAlign ? extraGap : parent.layout.spacing;
            cursor += childMainSize + margins + gap;
        }

        // FitContent / MaxContent / MinContent / Auto: expand parent to fit children on main axis
        if ((parent.widthMode == SizeMode::FitContent || parent.widthMode == SizeMode::MaxContent ||
             parent.widthMode == SizeMode::MinContent || parent.widthMode == SizeMode::Auto) && isRow) {
            float needed = cursor - parent.layout.spacing + parent.padding.left + parent.padding.right;
            if (parent.widthMode != SizeMode::Auto || needed > parent.computedRect.w)
                parent.computedRect.w = needed;
        }
        if ((parent.heightMode == SizeMode::FitContent || parent.heightMode == SizeMode::MaxContent ||
             parent.heightMode == SizeMode::MinContent || parent.heightMode == SizeMode::Auto) && !isRow) {
            float needed = cursor - parent.layout.spacing + parent.padding.top + parent.padding.bottom;
            if (parent.heightMode != SizeMode::Auto || needed > parent.computedRect.h)
                parent.computedRect.h = needed;
        }

        // Apply main-axis alignment (Center/End only - space-* handled above)
        if (parent.layout.mainAlign == Alignment::Center ||
            parent.layout.mainAlign == Alignment::End) {
            float totalSize = cursor - parent.layout.spacing;
            float availableSize = isRow ? contentRect.w : contentRect.h;
            float offset = 0.0f;

            if (parent.layout.mainAlign == Alignment::Center) {
                offset = (availableSize - totalSize) * 0.5f;
            } else if (parent.layout.mainAlign == Alignment::End) {
                offset = availableSize - totalSize;
            }

            if (offset > 0.0f) {
                for (int i = 0; i < parent.childCount; ++i) {
                    Widget* child = pool.Get(parent.children[i]);
                    if (!child || !child->IsLayoutVisible()) continue;
                    if (isRow) child->computedRect.x += offset;
                    else       child->computedRect.y += offset;
                }
            }
        }

        // FitContent/MinContent/MaxContent cross-axis: shrink to tallest/widest child
        if (isRow && (parent.heightMode == SizeMode::FitContent ||
                      parent.heightMode == SizeMode::MinContent ||
                      parent.heightMode == SizeMode::MaxContent)) {
            float maxCross = 0.0f;
            for (int i = 0; i < parent.childCount; ++i) {
                Widget* child = pool.Get(parent.children[i]);
                if (!child || !child->IsLayoutVisible()) continue;
                if (child->positionMode == PositionMode::Absolute) continue;
                float ch = child->computedRect.h + child->margin.top + child->margin.bottom;
                if (ch > maxCross) maxCross = ch;
            }
            parent.computedRect.h = maxCross + parent.padding.top + parent.padding.bottom;
            // Update contentRect for cross-axis alignment below
            contentRect.h = parent.computedRect.h - parent.padding.top - parent.padding.bottom;
        } else if (!isRow && (parent.widthMode == SizeMode::FitContent ||
                              parent.widthMode == SizeMode::MinContent ||
                              parent.widthMode == SizeMode::MaxContent)) {
            float maxCross = 0.0f;
            for (int i = 0; i < parent.childCount; ++i) {
                Widget* child = pool.Get(parent.children[i]);
                if (!child || !child->IsLayoutVisible()) continue;
                if (child->positionMode == PositionMode::Absolute) continue;
                float cw = child->computedRect.w + child->margin.left + child->margin.right;
                if (cw > maxCross) maxCross = cw;
            }
            parent.computedRect.w = maxCross + parent.padding.left + parent.padding.right;
            contentRect.w = parent.computedRect.w - parent.padding.left - parent.padding.right;
        }

        // Cross-axis alignment (per-child align-self overrides parent crossAlign)
        for (int i = 0; i < parent.childCount; ++i) {
            Widget* child = pool.Get(parent.children[i]);
            if (!child || !child->IsLayoutVisible()) continue;
            if (child->positionMode == PositionMode::Absolute) continue;

            // Resolve effective alignment: child's alignSelf overrides parent's crossAlign
            Alignment eff = parent.layout.crossAlign;
            if (child->alignSelf != AlignSelf::Auto) {
                switch (child->alignSelf) {
                    case AlignSelf::Start:   eff = Alignment::Start; break;
                    case AlignSelf::Center:  eff = Alignment::Center; break;
                    case AlignSelf::End:     eff = Alignment::End; break;
                    case AlignSelf::Stretch: eff = Alignment::Stretch; break;
                    default: break;
                }
            }

            if (eff == Alignment::Center) {
                if (isRow) {
                    float extra = contentRect.h - child->computedRect.h;
                    child->computedRect.y += extra * 0.5f;
                } else {
                    float extra = contentRect.w - child->computedRect.w;
                    child->computedRect.x += extra * 0.5f;
                }
            } else if (eff == Alignment::End) {
                if (isRow) {
                    child->computedRect.y = contentRect.y + contentRect.h - child->computedRect.h;
                } else {
                    child->computedRect.x = contentRect.x + contentRect.w - child->computedRect.w;
                }
            }
        }

        // Position absolute children relative to parent content rect
        for (int i = 0; i < parent.childCount; ++i) {
            Widget* child = pool.Get(parent.children[i]);
            if (!child || !child->IsLayoutVisible()) continue;
            if (child->positionMode != PositionMode::Absolute) continue;

            // Compute size normally
            ComputeRect(*child, contentRect);

            // Position using top/left/right/bottom offsets
            // When both left+right are set, stretch width to fill
            if (child->posLeft != 0.0f && child->posRight != 0.0f) {
                child->computedRect.x = contentRect.x + child->posLeft;
                child->computedRect.w = contentRect.w - child->posLeft - child->posRight;
            } else if (child->posRight != 0.0f) {
                child->computedRect.x = contentRect.x + contentRect.w - child->computedRect.w - child->posRight;
            } else {
                child->computedRect.x = contentRect.x + child->posLeft;
            }

            // When both top+bottom are set, stretch height to fill
            if (child->posTop != 0.0f && child->posBottom != 0.0f) {
                child->computedRect.y = contentRect.y + child->posTop;
                child->computedRect.h = contentRect.h - child->posTop - child->posBottom;
            } else if (child->posBottom != 0.0f) {
                child->computedRect.y = contentRect.y + contentRect.h - child->computedRect.h - child->posBottom;
            } else {
                child->computedRect.y = contentRect.y + child->posTop;
            }

            // Layout absolute child's children
            if (child->childCount > 0) {
                LayoutChildren(pool, *child);
            }
            child->flags.dirty = 0;
        }

        // ScrollView or overflow: scroll - apply scroll offset and track content extent
        if (parent.tag == WidgetTag::ScrollView || parent.flags.scrollable) {
            // Compute total content height for scrollbar
            float contentEnd = 0.0f;
            for (int i = 0; i < parent.childCount; ++i) {
                Widget* child = pool.Get(parent.children[i]);
                if (!child || !child->IsLayoutVisible()) continue;
                float bottom = child->computedRect.y + child->computedRect.h
                             + child->margin.bottom - contentRect.y;
                if (bottom > contentEnd) contentEnd = bottom;
            }
            parent.contentHeight = contentEnd;

            // Clamp scrollY to valid range
            float maxScroll = contentEnd - contentRect.h;
            if (maxScroll < 0.0f) maxScroll = 0.0f;
            if (parent.scrollY < -maxScroll) parent.scrollY = -maxScroll;
            if (parent.scrollY > 0.0f) parent.scrollY = 0.0f;

            // Offset all children by scroll amount
            if (parent.scrollY != 0.0f) {
                for (int i = 0; i < parent.childCount; ++i) {
                    Widget* child = pool.Get(parent.children[i]);
                    if (!child || !child->IsLayoutVisible()) continue;
                    OffsetSubtree(pool, parent.children[i], 0.0f, parent.scrollY);
                }
            }
        }
    }

    /// CSS Grid layout algorithm
    void LayoutGrid(WidgetPool& pool, Widget& parent) {
        Rect contentRect = {
            parent.computedRect.x + parent.padding.left,
            parent.computedRect.y + parent.padding.top,
            parent.computedRect.w - parent.padding.left - parent.padding.right,
            parent.computedRect.h - parent.padding.top - parent.padding.bottom
        };

        const auto& g = parent.grid;
        int cols = std::max(1, g.columnCount);
        int rows = std::max(1, g.rowCount);

        // Collect visible children
        struct GridChild { int index; int col; int row; int colSpan; int rowSpan; };
        std::vector<GridChild> children;
        children.reserve(static_cast<size_t>(parent.childCount));

        for (int i = 0; i < parent.childCount; ++i) {
            Widget* child = pool.Get(parent.children[i]);
            if (!child || !child->IsLayoutVisible()) continue;
            if (child->positionMode == PositionMode::Absolute) continue;
            children.push_back({parent.children[i], child->gridPlace.column,
                                child->gridPlace.row, child->gridPlace.columnSpan,
                                child->gridPlace.rowSpan});
        }

        // Auto-place children that don't have explicit positions
        {
            // Build occupancy grid
            int maxRow = rows;
            for (auto& c : children) {
                if (c.row > 0 && c.row + c.rowSpan - 1 > maxRow)
                    maxRow = c.row + c.rowSpan - 1;
            }
            std::vector<std::vector<bool>> occupied(static_cast<size_t>(maxRow + (int)children.size()),
                                                     std::vector<bool>(static_cast<size_t>(cols), false));
            // Mark explicitly placed
            for (auto& c : children) {
                if (c.col > 0 && c.row > 0) {
                    for (int r = c.row - 1; r < c.row - 1 + c.rowSpan && r < (int)occupied.size(); ++r)
                        for (int cc = c.col - 1; cc < c.col - 1 + c.colSpan && cc < cols; ++cc)
                            occupied[static_cast<size_t>(r)][static_cast<size_t>(cc)] = true;
                }
            }
            // Auto-place remaining
            int autoRow = 0, autoCol = 0;
            for (auto& c : children) {
                if (c.col > 0 && c.row > 0) continue;
                // Find next available slot
                while (true) {
                    if (autoRow >= (int)occupied.size())
                        occupied.emplace_back(std::vector<bool>(static_cast<size_t>(cols), false));
                    if (autoCol + c.colSpan <= cols) {
                        bool fits = true;
                        for (int r = autoRow; r < autoRow + c.rowSpan && fits; ++r) {
                            if (r >= (int)occupied.size())
                                occupied.emplace_back(std::vector<bool>(static_cast<size_t>(cols), false));
                            for (int cc = autoCol; cc < autoCol + c.colSpan && fits; ++cc)
                                if (occupied[static_cast<size_t>(r)][static_cast<size_t>(cc)]) fits = false;
                        }
                        if (fits) {
                            c.col = autoCol + 1;
                            c.row = autoRow + 1;
                            for (int r = autoRow; r < autoRow + c.rowSpan; ++r)
                                for (int cc = autoCol; cc < autoCol + c.colSpan; ++cc)
                                    occupied[static_cast<size_t>(r)][static_cast<size_t>(cc)] = true;
                            break;
                        }
                    }
                    autoCol++;
                    if (autoCol >= cols) { autoCol = 0; autoRow++; }
                }
            }
            // Update actual row count from auto-placement
            for (auto& c : children)
                if (c.row + c.rowSpan - 1 > rows) rows = c.row + c.rowSpan - 1;
        }

        // Resolve track sizes
        auto resolveTracks = [](const GridTrackDef* defs, int defCount, int trackCount,
                                float available, float gap) -> std::vector<float> {
            std::vector<float> sizes(static_cast<size_t>(trackCount), 0.0f);
            float totalGaps = gap * std::max(0, trackCount - 1);
            float remaining = available - totalGaps;
            float totalFr = 0.0f;
            int autoCount = 0;

            // First pass: assign fixed sizes
            for (int i = 0; i < trackCount; ++i) {
                GridTrackDef def;
                if (i < defCount) def = defs[i];
                switch (def.type) {
                    case GridTrackDef::Type::Px:
                        sizes[static_cast<size_t>(i)] = def.value;
                        remaining -= def.value;
                        break;
                    case GridTrackDef::Type::Fr:
                        totalFr += def.value;
                        break;
                    case GridTrackDef::Type::Auto:
                    case GridTrackDef::Type::MinContent:
                    case GridTrackDef::Type::MaxContent:
                        autoCount++;
                        break;
                }
            }

            // Distribute auto tracks equally from remaining (before fr)
            if (autoCount > 0 && totalFr == 0.0f) {
                float autoSize = std::max(0.0f, remaining / autoCount);
                for (int i = 0; i < trackCount; ++i) {
                    GridTrackDef def;
                    if (i < defCount) def = defs[i];
                    if (def.type == GridTrackDef::Type::Auto ||
                        def.type == GridTrackDef::Type::MinContent ||
                        def.type == GridTrackDef::Type::MaxContent) {
                        sizes[static_cast<size_t>(i)] = autoSize;
                        remaining -= autoSize;
                    }
                }
            } else if (autoCount > 0) {
                // With fr tracks present, auto tracks get minimum size
                float autoSize = 20.0f; // minimum auto track size
                for (int i = 0; i < trackCount; ++i) {
                    GridTrackDef def;
                    if (i < defCount) def = defs[i];
                    if (def.type == GridTrackDef::Type::Auto ||
                        def.type == GridTrackDef::Type::MinContent ||
                        def.type == GridTrackDef::Type::MaxContent) {
                        sizes[static_cast<size_t>(i)] = autoSize;
                        remaining -= autoSize;
                    }
                }
            }

            // Distribute fr tracks
            if (totalFr > 0.0f && remaining > 0.0f) {
                float perFr = remaining / totalFr;
                for (int i = 0; i < trackCount; ++i) {
                    GridTrackDef def;
                    if (i < defCount) def = defs[i];
                    if (def.type == GridTrackDef::Type::Fr) {
                        sizes[static_cast<size_t>(i)] = def.value * perFr;
                    }
                }
            }

            return sizes;
        };

        auto colSizes = resolveTracks(g.columns, g.columnCount, cols, contentRect.w, g.columnGap);
        auto rowSizes = resolveTracks(g.rows, g.rowCount, rows, contentRect.h, g.rowGap);

        // Compute column/row positions (cumulative)
        std::vector<float> colPos(static_cast<size_t>(cols) + 1, 0.0f);
        colPos[0] = contentRect.x;
        for (int i = 0; i < cols; ++i)
            colPos[static_cast<size_t>(i) + 1] = colPos[static_cast<size_t>(i)] + colSizes[static_cast<size_t>(i)] + (i < cols - 1 ? g.columnGap : 0.0f);

        std::vector<float> rowPos(static_cast<size_t>(rows) + 1, 0.0f);
        rowPos[0] = contentRect.y;
        for (int i = 0; i < rows; ++i)
            rowPos[static_cast<size_t>(i) + 1] = rowPos[static_cast<size_t>(i)] + rowSizes[static_cast<size_t>(i)] + (i < rows - 1 ? g.rowGap : 0.0f);

        // Place children into grid cells
        for (auto& gc : children) {
            Widget* child = pool.Get(gc.index);
            if (!child) continue;

            int c0 = std::max(0, gc.col - 1);
            int r0 = std::max(0, gc.row - 1);
            int c1 = std::min(cols, c0 + gc.colSpan);
            int r1 = std::min(rows, r0 + gc.rowSpan);

            float cx = colPos[static_cast<size_t>(c0)];
            float cy = rowPos[static_cast<size_t>(r0)];
            float cw = colPos[static_cast<size_t>(c1)] - cx;
            float ch = rowPos[static_cast<size_t>(r1)] - cy;

            // Remove gap from the end if spanning
            if (gc.colSpan > 1) cw -= g.columnGap * 0; // gap is between, already accounted
            if (gc.rowSpan > 1) ch -= g.rowGap * 0;

            // Apply child margin
            cx += child->margin.left;
            cy += child->margin.top;
            cw -= child->margin.left + child->margin.right;
            ch -= child->margin.top + child->margin.bottom;

            child->computedRect = {cx, cy, std::max(0.0f, cw), std::max(0.0f, ch)};

            // Recurse into children
            if (child->childCount > 0) {
                LayoutChildren(pool, *child);
            }
            child->flags.dirty = 0;
        }

        // Handle absolute positioned children
        for (int i = 0; i < parent.childCount; ++i) {
            Widget* child = pool.Get(parent.children[i]);
            if (!child || !child->IsLayoutVisible()) continue;
            if (child->positionMode != PositionMode::Absolute) continue;

            ComputeRect(*child, contentRect);
            if (child->childCount > 0) LayoutChildren(pool, *child);
            child->flags.dirty = 0;
        }
    }

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
