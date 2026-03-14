// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file timeline.cpp
 * @brief Timeline widget implementation.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "timeline.hpp"

namespace koilo {
namespace ui {

// ============================================================================
// Build the timeline inside a parent widget.
int Timeline::Build(UIContext& ctx, int parentIdx, const char* id, float height) {
    ctx_ = &ctx;

    char canvasId[64];
    snprintf(canvasId, 64, "%s_timeline", id);
    canvasIdx_ = ctx.CreateCanvas2D(canvasId,
        [this](void* rawCtx) { Paint(rawCtx); });
    ctx.SetParent(canvasIdx_, parentIdx);

    Widget* w = ctx.Pool().Get(canvasIdx_);
    w->localH = height;
    w->heightMode = SizeMode::Fixed;
    w->widthMode = SizeMode::FillRemaining;

    return canvasIdx_;
}

// ============================================================================
// Handle click/drag to scrub timeline.
void Timeline::HandleInput(float localX, float canvasW, bool pressed) {
    if (!pressed || canvasW <= 0) return;
    float margin = 10.0f;
    float plotW = canvasW - margin * 2;
    if (plotW <= 0) return;
    float t = (localX - margin) / plotW;
    t = std::max(0.0f, std::min(1.0f, t));
    int frame = startFrame_ + static_cast<int>(t * (endFrame_ - startFrame_));
    SetCurrentFrame(frame);
}

// ============================================================================
// Paint the timeline canvas.
void Timeline::Paint(void* rawCtx) {
    auto* dc = static_cast<CanvasDrawContext*>(rawCtx);
    float w = dc->Width();
    float h = dc->Height();
    if (w <= 0 || h <= 0) return;

    float margin = 10.0f;
    float plotW = w - margin * 2;

    // Background
    dc->DrawRect(0, 0, w, h, {35, 35, 42, 255});

    // Frame marks
    int range = endFrame_ - startFrame_;
    if (range <= 0) return;

    // Calculate tick spacing
    int majorTick = 10;
    if (range > 200) majorTick = 50;
    else if (range > 100) majorTick = 20;

    Color4 minorCol{60, 60, 70, 150};
    Color4 majorCol{100, 100, 115, 200};
    Color4 textCol{160, 160, 175, 255};

    for (int f = startFrame_; f <= endFrame_; ++f) {
        float t = static_cast<float>(f - startFrame_) / range;
        float fx = margin + t * plotW;

        if (f % majorTick == 0) {
            dc->DrawLine(fx, h * 0.3f, fx, h * 0.85f, 1.0f, majorCol);
            char label[16];
            snprintf(label, 16, "%d", f);
            dc->DrawText(fx - 6, 2.0f, label, textCol, 10.0f);
        } else if (f % (majorTick / 5 > 0 ? majorTick / 5 : 1) == 0) {
            dc->DrawLine(fx, h * 0.55f, fx, h * 0.85f, 1.0f, minorCol);
        }
    }

    // Keyframe markers
    for (const auto& kf : keyframes_) {
        if (kf.frame < startFrame_ || kf.frame > endFrame_) continue;
        float t = static_cast<float>(kf.frame - startFrame_) / range;
        float kx = margin + t * plotW;
        float ky = h * 0.7f;
        // Diamond shape via two triangles
        float sz = 4.0f;
        dc->DrawTriangle(kx, ky - sz, kx + sz, ky, kx, ky + sz, kf.color);
        dc->DrawTriangle(kx, ky - sz, kx - sz, ky, kx, ky + sz, kf.color);
    }

    // Playhead
    float pt = static_cast<float>(currentFrame_ - startFrame_) / range;
    float px = margin + pt * plotW;
    dc->DrawLine(px, 0, px, h, 2.0f, {255, 80, 80, 230});
    dc->DrawTriangle(px - 5, 0, px + 5, 0, px, 8, {255, 80, 80, 230});
}

} // namespace ui
} // namespace koilo
