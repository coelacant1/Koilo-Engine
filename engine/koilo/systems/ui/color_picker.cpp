// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file color_picker.cpp
 * @brief Color picker floating panel implementation.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "color_picker.hpp"

#include <cmath>
#include <cstdio>

namespace koilo {
namespace ui {

// ============================================================================
// Lifecycle
// ============================================================================

void ColorPicker::Build(UIContext& ctx) {
    ctx_ = &ctx;

    // Floating panel (hidden by default).
    panelIdx_ = ctx.CreateFloatingPanel("__color_picker_panel",
                                         "Color Picker",
                                         100.0f, 100.0f, 240.0f, 310.0f);
    Widget* panel = ctx.Pool().Get(panelIdx_);
    panel->flags.visible = false;
    panel->layout.spacing = 4.0f;
    panel->padding.left   = 6.0f;
    panel->padding.right  = 6.0f;
    panel->padding.bottom = 6.0f;

    // SV canvas (saturation on X, value on Y).
    svCanvasIdx_ = ctx.CreateCanvas2D("__cp_sv_canvas",
        [this](void* rawCtx) { PaintSVCanvas(rawCtx); });
    ctx.SetParent(svCanvasIdx_, panelIdx_);
    Widget* svw = ctx.Pool().Get(svCanvasIdx_);
    svw->localH     = 160.0f;
    svw->heightMode = SizeMode::Fixed;
    svw->widthMode  = SizeMode::FillRemaining;

    // Hue bar canvas.
    hueBarIdx_ = ctx.CreateCanvas2D("__cp_hue_bar",
        [this](void* rawCtx) { PaintHueBar(rawCtx); });
    ctx.SetParent(hueBarIdx_, panelIdx_);
    Widget* hw = ctx.Pool().Get(hueBarIdx_);
    hw->localH     = 16.0f;
    hw->heightMode = SizeMode::Fixed;
    hw->widthMode  = SizeMode::FillRemaining;

    // Alpha bar canvas.
    alphaBarIdx_ = ctx.CreateCanvas2D("__cp_alpha_bar",
        [this](void* rawCtx) { PaintAlphaBar(rawCtx); });
    ctx.SetParent(alphaBarIdx_, panelIdx_);
    Widget* aw = ctx.Pool().Get(alphaBarIdx_);
    aw->localH     = 16.0f;
    aw->heightMode = SizeMode::Fixed;
    aw->widthMode  = SizeMode::FillRemaining;

    // Bottom row: hex input + preview swatch.
    bottomRowIdx_ = ctx.CreatePanel("__cp_bottom_row");
    ctx.SetParent(bottomRowIdx_, panelIdx_);
    Widget* br = ctx.Pool().Get(bottomRowIdx_);
    br->localH            = 28.0f;
    br->heightMode        = SizeMode::Fixed;
    br->widthMode         = SizeMode::FillRemaining;
    br->layout.direction  = LayoutDirection::Row;
    br->layout.spacing    = 4.0f;

    hexFieldIdx_ = ctx.CreateTextField("__cp_hex", "#FFFFFF");
    ctx.SetParent(hexFieldIdx_, bottomRowIdx_);
    Widget* hf = ctx.Pool().Get(hexFieldIdx_);
    hf->widthMode  = SizeMode::FillRemaining;
    hf->localH     = 24.0f;
    hf->heightMode = SizeMode::Fixed;

    previewIdx_ = ctx.CreateWidget(WidgetTag::ColorField, "__cp_preview");
    ctx.SetParent(previewIdx_, bottomRowIdx_);
    Widget* pw = ctx.Pool().Get(previewIdx_);
    pw->localW     = 40.0f;
    pw->localH     = 24.0f;
    pw->widthMode  = SizeMode::Fixed;
    pw->heightMode = SizeMode::Fixed;
    pw->colorValue = {255, 255, 255, 255};

    // OK button row.
    btnRowIdx_ = ctx.CreatePanel("__cp_btn_row");
    ctx.SetParent(btnRowIdx_, panelIdx_);
    Widget* brow = ctx.Pool().Get(btnRowIdx_);
    brow->localH            = 28.0f;
    brow->heightMode        = SizeMode::Fixed;
    brow->widthMode         = SizeMode::FillRemaining;
    brow->layout.direction  = LayoutDirection::Row;
    brow->layout.spacing    = 4.0f;

    okBtnIdx_ = ctx.CreateButton("__cp_ok_btn", "OK");
    ctx.SetParent(okBtnIdx_, btnRowIdx_);
    Widget* okBtn = ctx.Pool().Get(okBtnIdx_);
    okBtn->widthMode  = SizeMode::FillRemaining;
    okBtn->localH     = 24.0f;
    okBtn->heightMode = SizeMode::Fixed;
    okBtn->onClickCpp = [this](Widget&) { Confirm(); };

    built_ = true;
}

// ============================================================================
// Open / Close / Confirm
// ============================================================================

void ColorPicker::Open(int sourceIdx, float screenX, float screenY) {
    if (!built_ || !ctx_) return;
    sourceIdx_ = sourceIdx;
    Widget* src = ctx_->Pool().Get(sourceIdx);
    if (!src) return;

    currentColor_ = src->colorValue;
    currentColor_.ToHSV(hue_, sat_, val_);
    alpha_ = currentColor_.a;

    UpdateHexField();
    UpdatePreview();

    Widget* panel = ctx_->Pool().Get(panelIdx_);
    if (panel) {
        panel->posLeft = screenX;
        panel->posTop  = screenY;
        panel->flags.visible = true;
    }
    ctx_->BringToFront(panelIdx_);
    ctx_->SetRenderDirty();
    open_ = true;
}

void ColorPicker::Close() {
    if (!ctx_) return;
    Widget* panel = ctx_->Pool().Get(panelIdx_);
    if (panel) {
        panel->flags.visible = false;
        panel->zOrder = 0;
    }
    open_ = false;
    activeCanvas_ = -1;
    sourceIdx_ = -1;
    ctx_->SetRenderDirty();
}

void ColorPicker::Confirm() {
    if (!open_ || !ctx_) return;
    UpdateSource();
    Close();
}

bool ColorPicker::IsOpen() const {
    if (!open_ || !ctx_ || panelIdx_ < 0) return false;
    const Widget* w = ctx_->Pool().Get(panelIdx_);
    return w && w->flags.visible;
}

// ============================================================================
// Pointer Interaction
// ============================================================================

void ColorPicker::HandlePointerDown(float px, float py) {
    if (!open_ || !ctx_) return;

    auto contains = [&](int idx) {
        const Widget* w = ctx_->Pool().Get(idx);
        return w && w->computedRect.Contains(px, py);
    };

    if (contains(svCanvasIdx_))       activeCanvas_ = 0;
    else if (contains(hueBarIdx_))    activeCanvas_ = 1;
    else if (contains(alphaBarIdx_))  activeCanvas_ = 2;
    else return;

    UpdateCanvasValue(activeCanvas_, px, py);
    ApplyColor();
}

void ColorPicker::HandlePointerDrag(float px, float py) {
    if (activeCanvas_ < 0 || !ctx_) return;
    UpdateCanvasValue(activeCanvas_, px, py);
    ApplyColor();
}

void ColorPicker::HandlePointerUp() {
    activeCanvas_ = -1;
}

void ColorPicker::UpdateCanvasValue(int canvas, float px, float py) {
    auto clamp01 = [](float v) { return std::max(0.0f, std::min(1.0f, v)); };

    if (canvas == 0) {
        const Widget* sv = ctx_->Pool().Get(svCanvasIdx_);
        if (!sv) return;
        Rect r = sv->computedRect;
        sat_ = clamp01((px - r.x) / r.w);
        val_ = 1.0f - clamp01((py - r.y) / r.h);
    } else if (canvas == 1) {
        const Widget* hb = ctx_->Pool().Get(hueBarIdx_);
        if (!hb) return;
        Rect r = hb->computedRect;
        hue_ = clamp01((px - r.x) / r.w) * 360.0f;
    } else if (canvas == 2) {
        const Widget* ab = ctx_->Pool().Get(alphaBarIdx_);
        if (!ab) return;
        Rect r = ab->computedRect;
        alpha_ = static_cast<uint8_t>(clamp01((px - r.x) / r.w) * 255.0f);
    }
}

void ColorPicker::ApplyHexInput() {
    if (!ctx_ || hexFieldIdx_ < 0) return;
    Widget* hf = ctx_->Pool().Get(hexFieldIdx_);
    if (!hf) return;
    const char* text = ctx_->Strings().Lookup(hf->textId);
    if (!text) return;
    Color4 c = Color4::FromHex(text);
    c.ToHSV(hue_, sat_, val_);
    alpha_ = c.a;
    ApplyColor();
}

// ============================================================================
// Internal State
// ============================================================================

void ColorPicker::ApplyColor() {
    currentColor_ = Color4::FromHSV(hue_, sat_, val_, alpha_);
    UpdatePreview();
    UpdateHexField();
    if (ctx_) ctx_->SetRenderDirty();
}

void ColorPicker::UpdatePreview() {
    if (!ctx_ || previewIdx_ < 0) return;
    Widget* pw = ctx_->Pool().Get(previewIdx_);
    if (pw) pw->colorValue = currentColor_;
}

void ColorPicker::UpdateHexField() {
    if (!ctx_ || hexFieldIdx_ < 0) return;
    char hex[10];
    if (alpha_ == 255) {
        snprintf(hex, sizeof(hex), "#%02X%02X%02X",
                 currentColor_.r, currentColor_.g, currentColor_.b);
    } else {
        snprintf(hex, sizeof(hex), "#%02X%02X%02X%02X",
                 currentColor_.r, currentColor_.g, currentColor_.b, alpha_);
    }
    Widget* hf = ctx_->Pool().Get(hexFieldIdx_);
    if (hf) hf->textId = ctx_->Strings().Intern(hex);
}

void ColorPicker::UpdateSource() {
    if (!ctx_ || sourceIdx_ < 0) return;
    Widget* src = ctx_->Pool().Get(sourceIdx_);
    if (src) {
        src->colorValue = currentColor_;
        if (src->onChangeCpp) src->onChangeCpp(*src);
    }
}

// ============================================================================
// Paint Callbacks
// ============================================================================

void ColorPicker::PaintSVCanvas(void* rawCtx) {
    auto* dc = static_cast<CanvasDrawContext*>(rawCtx);
    float w = dc->Width();
    float h = dc->Height();
    if (w <= 0 || h <= 0) return;

    constexpr int STEPS = 16;
    float stepW = w / STEPS;
    float stepH = h / STEPS;
    for (int sy = 0; sy < STEPS; ++sy) {
        float v = 1.0f - (sy + 0.5f) / STEPS;
        for (int sx = 0; sx < STEPS; ++sx) {
            float s = (sx + 0.5f) / STEPS;
            Color4 c = Color4::FromHSV(hue_, s, v);
            dc->DrawRect(sx * stepW, sy * stepH, stepW + 1, stepH + 1, c);
        }
    }

    float cx = sat_ * w;
    float cy = (1.0f - val_) * h;
    Color4 cross = (val_ > 0.5f) ? Color4{0, 0, 0, 200} : Color4{255, 255, 255, 200};
    dc->DrawLine(cx - 6, cy, cx + 6, cy, 1.0f, cross);
    dc->DrawLine(cx, cy - 6, cx, cy + 6, 1.0f, cross);
}

void ColorPicker::PaintHueBar(void* rawCtx) {
    auto* dc = static_cast<CanvasDrawContext*>(rawCtx);
    float w = dc->Width();
    float h = dc->Height();
    if (w <= 0 || h <= 0) return;

    constexpr int STEPS = 36;
    float stepW = w / STEPS;
    for (int i = 0; i < STEPS; ++i) {
        float hVal = (i + 0.5f) / STEPS * 360.0f;
        Color4 c = Color4::FromHSV(hVal, 1.0f, 1.0f);
        dc->DrawRect(i * stepW, 0, stepW + 1, h, c);
    }

    float ix = (hue_ / 360.0f) * w;
    dc->DrawRect(ix - 1, 0, 3, h, {255, 255, 255, 220});
}

void ColorPicker::PaintAlphaBar(void* rawCtx) {
    auto* dc = static_cast<CanvasDrawContext*>(rawCtx);
    float w = dc->Width();
    float h = dc->Height();
    if (w <= 0 || h <= 0) return;

    float checkSz = h * 0.5f;
    for (float cx = 0; cx < w; cx += checkSz) {
        for (float cy = 0; cy < h; cy += checkSz) {
            int idx = static_cast<int>(cx / checkSz) + static_cast<int>(cy / checkSz);
            Color4 bg = (idx % 2 == 0) ? Color4{180, 180, 180, 255}
                                        : Color4{120, 120, 120, 255};
            dc->DrawRect(cx, cy, checkSz + 1, checkSz + 1, bg);
        }
    }

    constexpr int STEPS = 16;
    float stepW = w / STEPS;
    Color4 base = Color4::FromHSV(hue_, sat_, val_);
    for (int i = 0; i < STEPS; ++i) {
        float t = (i + 0.5f) / STEPS;
        Color4 c = base;
        c.a = static_cast<uint8_t>(t * 255.0f);
        dc->DrawRect(i * stepW, 0, stepW + 1, h, c);
    }

    float ix = (alpha_ / 255.0f) * w;
    dc->DrawRect(ix - 1, 0, 3, h, {255, 255, 255, 220});
}

} // namespace ui
} // namespace koilo
