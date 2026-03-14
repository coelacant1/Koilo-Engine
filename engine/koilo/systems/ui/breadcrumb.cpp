// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file breadcrumb.cpp
 * @brief Breadcrumb navigation widget implementation.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "breadcrumb.hpp"

#include <cstdio>

namespace koilo {
namespace ui {

// ============================================================================
// Lifecycle
// ============================================================================

// Build the breadcrumb bar inside a parent widget.
int Breadcrumb::Build(UIContext& ctx, int parentIdx, const char* id) {
    rowIdx_ = ctx.CreatePanel(id);
    if (rowIdx_ < 0) return -1;
    ctx.SetParent(rowIdx_, parentIdx);
    Widget* row = ctx.Pool().Get(rowIdx_);
    row->layout.direction = LayoutDirection::Row;
    row->layout.crossAlign = Alignment::Center;
    row->widthMode  = SizeMode::Percent;
    row->localW     = 100.0f;
    row->heightMode = SizeMode::Fixed;
    row->localH     = 24.0f;
    ctx_ = &ctx;
    return rowIdx_;
}

// ============================================================================
// Segment Rebuilding
// ============================================================================

// Rebuild breadcrumb segments from the current path.
void Breadcrumb::RebuildSegments() {
    UIContext& ctx = *ctx_;

    // Remove existing children
    Widget* row = ctx.Pool().Get(rowIdx_);
    if (!row) return;
    for (int i = 0; i < row->childCount; ++i) {
        int ci = row->children[i];
        if (ci >= 0) ctx.Pool().Free(ci);
    }
    row->childCount = 0;

    // Split path
    segments_.clear();
    std::string seg;
    for (char c : currentPath_) {
        if (c == '/' || c == '\\') {
            if (!seg.empty()) {
                segments_.push_back(seg);
                seg.clear();
            }
        } else {
            seg += c;
        }
    }
    if (!seg.empty()) segments_.push_back(seg);

    // Limit display: if too many, show "..." + last N
    constexpr int MAX_VISIBLE = 6;
    int startIdx = 0;
    bool truncated = false;
    if (static_cast<int>(segments_.size()) > MAX_VISIBLE) {
        startIdx = static_cast<int>(segments_.size()) - MAX_VISIBLE;
        truncated = true;
    }

    // Root button
    {
        char btnId[64];
        std::snprintf(btnId, sizeof(btnId), "%s_root",
                      ctx.Strings().Lookup(ctx.Pool().Get(rowIdx_)->id));
        int btn = ctx.CreateButton(btnId, "/");
        ctx.SetParent(btn, rowIdx_);
        Widget* bw = ctx.Pool().Get(btn);
        bw->widthMode  = SizeMode::Auto;
        bw->heightMode = SizeMode::Percent;
        bw->localH = 100.0f;
        bw->padding = {2, 2, 4, 4};
        bw->onClickCpp = [this](Widget&) {
            if (onNavigate_) onNavigate_("/");
        };
    }

    if (truncated) {
        char eid[64];
        std::snprintf(eid, sizeof(eid), "%s_ellip",
                      ctx.Strings().Lookup(ctx.Pool().Get(rowIdx_)->id));
        int lbl = ctx.CreateLabel(eid, "...");
        ctx.SetParent(lbl, rowIdx_);
        Widget* lw = ctx.Pool().Get(lbl);
        lw->widthMode = SizeMode::Auto;
        lw->padding = {0, 0, 2, 2};
    }

    for (int i = startIdx; i < static_cast<int>(segments_.size()); ++i) {
        // Separator
        if (i > startIdx || !truncated) {
            char sepId[64];
            std::snprintf(sepId, sizeof(sepId), "%s_sep%d",
                          ctx.Strings().Lookup(ctx.Pool().Get(rowIdx_)->id), i);
            int sep = ctx.CreateLabel(sepId, "\xe2\x80\xba"); // "›"
            ctx.SetParent(sep, rowIdx_);
            Widget* sw = ctx.Pool().Get(sep);
            sw->widthMode = SizeMode::Auto;
            sw->padding = {0, 0, 2, 2};
        }

        // Segment button
        char btnId[64];
        std::snprintf(btnId, sizeof(btnId), "%s_s%d",
                      ctx.Strings().Lookup(ctx.Pool().Get(rowIdx_)->id), i);
        int btn = ctx.CreateButton(btnId, segments_[i].c_str());
        ctx.SetParent(btn, rowIdx_);
        Widget* bw = ctx.Pool().Get(btn);
        bw->widthMode  = SizeMode::Auto;
        bw->heightMode = SizeMode::Percent;
        bw->localH = 100.0f;
        bw->padding = {2, 2, 4, 4};

        int navIdx = i; // capture by value
        bw->onClickCpp = [this, navIdx](Widget&) {
            if (onNavigate_) {
                std::string navPath;
#if !defined(_WIN32)
                navPath += '/';
#endif
                for (int j = 0; j <= navIdx; ++j) {
                    navPath += segments_[j];
                    if (j < navIdx) navPath += '/';
                }
                onNavigate_(navPath);
            }
        };
    }
}

} // namespace ui
} // namespace koilo
