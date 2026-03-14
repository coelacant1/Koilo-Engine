// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file curve_editor.cpp
 * @brief Curve editor widget implementation.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "curve_editor.hpp"

namespace koilo {
namespace ui {

// ============================================================================
// Widget Construction
// ============================================================================

// Build the curve editor canvas inside a parent widget.
int CurveEditor::Build(UIContext& ctx, int parentIdx, const char* id, float height) {
    ctx_ = &ctx;

    char canvasId[64];
    snprintf(canvasId, 64, "%s_curve_canvas", id);
    canvasIdx_ = ctx.CreateCanvas2D(canvasId,
        [this](void* rawCtx) { Paint(rawCtx); });
    ctx.SetParent(canvasIdx_, parentIdx);

    Widget* w = ctx.Pool().Get(canvasIdx_);
    w->localH = height;
    w->heightMode = SizeMode::Fixed;
    w->widthMode = SizeMode::FillRemaining;

    // Default ease-in-out curve
    if (points_.empty()) {
        points_.push_back({0.0f, 0.0f, 0.0f, 0.0f, 0.15f, 0.0f});
        points_.push_back({1.0f, 1.0f, -0.15f, 0.0f, 0.0f, 0.0f});
    }

    return canvasIdx_;
}

// ============================================================================
// Point Management
// ============================================================================

// Remove the selected point (if not an endpoint).
void CurveEditor::RemoveSelected() {
    if (points_.size() <= 2) return;
    points_.erase(
        std::remove_if(points_.begin(), points_.end(),
            [](const CurvePoint& p) { return p.selected; }),
        points_.end());
    // Protect endpoints
    if (points_.empty() || points_.front().x > 0.01f) {
        points_.insert(points_.begin(), {0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f});
    }
    if (points_.back().x < 0.99f) {
        points_.push_back({1.0f, 1.0f, -0.1f, 0.0f, 0.0f, 0.0f});
    }
}

// ============================================================================
// Evaluation
// ============================================================================

// Evaluate the curve at normalized x in [0,1].
float CurveEditor::Evaluate(float x) const {
    if (points_.size() < 2) return x;
    if (x <= points_.front().x) return points_.front().y;
    if (x >= points_.back().x) return points_.back().y;

    // Find segment
    for (size_t i = 0; i + 1 < points_.size(); ++i) {
        const auto& p0 = points_[i];
        const auto& p1 = points_[i + 1];
        if (x >= p0.x && x <= p1.x) {
            float segLen = p1.x - p0.x;
            if (segLen < 1e-6f) return p0.y;
            float t = (x - p0.x) / segLen;
            // Cubic bezier
            float cp0y = p0.y;
            float cp1y = p0.y + p0.handleOutY * segLen;
            float cp2y = p1.y + p1.handleInY * segLen;
            float cp3y = p1.y;
            return CubicBezier(t, cp0y, cp1y, cp2y, cp3y);
        }
    }
    return x;
}

// ============================================================================
// Input Handling
// ============================================================================

// Handle mouse interaction in canvas-local coordinates.
void CurveEditor::HandleInput(float localX, float localY, float canvasW, float canvasH,
                               bool pressed, bool justPressed) {
    float margin = 20.0f;
    float plotW = canvasW - margin * 2;
    float plotH = canvasH - margin * 2;
    float nx = (localX - margin) / plotW;
    float ny = 1.0f - (localY - margin) / plotH;

    if (justPressed) {
        dragging_ = -1;
        dragHandle_ = 0;
        // Check for point hit
        for (size_t i = 0; i < points_.size(); ++i) {
            float dx = nx - points_[i].x;
            float dy = ny - points_[i].y;
            if (dx * dx + dy * dy < 0.02f * 0.02f) {
                dragging_ = static_cast<int>(i);
                dragHandle_ = 0; // main point
                for (auto& p : points_) p.selected = false;
                points_[i].selected = true;
                break;
            }
            // Check handle out
            float hox = points_[i].x + points_[i].handleOutX;
            float hoy = points_[i].y + points_[i].handleOutY;
            if ((nx - hox) * (nx - hox) + (ny - hoy) * (ny - hoy) < 0.02f * 0.02f) {
                dragging_ = static_cast<int>(i);
                dragHandle_ = 1; // out handle
                break;
            }
            // Check handle in
            float hix = points_[i].x + points_[i].handleInX;
            float hiy = points_[i].y + points_[i].handleInY;
            if ((nx - hix) * (nx - hix) + (ny - hiy) * (ny - hiy) < 0.02f * 0.02f) {
                dragging_ = static_cast<int>(i);
                dragHandle_ = -1; // in handle
                break;
            }
        }
        // Double-click could add a point (simplified: add on empty area click)
        if (dragging_ < 0 && nx >= 0 && nx <= 1 && ny >= 0 && ny <= 1) {
            for (auto& p : points_) p.selected = false;
        }
    }

    if (pressed && dragging_ >= 0) {
        auto& pt = points_[dragging_];
        if (dragHandle_ == 0) {
            // Move point (endpoints locked on x)
            if (dragging_ > 0 && dragging_ < static_cast<int>(points_.size()) - 1) {
                pt.x = std::max(0.0f, std::min(1.0f, nx));
            }
            pt.y = std::max(-0.2f, std::min(1.2f, ny));
        } else if (dragHandle_ == 1) {
            pt.handleOutX = nx - pt.x;
            pt.handleOutY = ny - pt.y;
        } else {
            pt.handleInX = nx - pt.x;
            pt.handleInY = ny - pt.y;
        }
    }

    if (!pressed) dragging_ = -1;
}

// ============================================================================
// Rendering
// ============================================================================

// Paint the curve editor onto the canvas.
void CurveEditor::Paint(void* rawCtx) {
    auto* dc = static_cast<CanvasDrawContext*>(rawCtx);
    float w = dc->Width();
    float h = dc->Height();
    if (w <= 0 || h <= 0) return;

    float margin = 20.0f;
    float plotW = w - margin * 2;
    float plotH = h - margin * 2;

    // Background
    dc->DrawRect(0, 0, w, h, {30, 30, 35, 255});
    dc->DrawRect(margin, margin, plotW, plotH, {40, 40, 48, 255});

    // Grid lines
    Color4 gridCol{60, 60, 70, 120};
    for (int i = 1; i < 4; ++i) {
        float gx = margin + plotW * i / 4.0f;
        float gy = margin + plotH * i / 4.0f;
        dc->DrawLine(gx, margin, gx, margin + plotH, 1.0f, gridCol);
        dc->DrawLine(margin, gy, margin + plotW, gy, 1.0f, gridCol);
    }

    // Curve
    Color4 curveCol{100, 200, 255, 255};
    constexpr int SAMPLES = 64;
    for (int i = 0; i < SAMPLES; ++i) {
        float x0 = static_cast<float>(i) / SAMPLES;
        float x1 = static_cast<float>(i + 1) / SAMPLES;
        float y0 = Evaluate(x0);
        float y1 = Evaluate(x1);
        float sx0 = margin + x0 * plotW;
        float sy0 = margin + (1.0f - y0) * plotH;
        float sx1 = margin + x1 * plotW;
        float sy1 = margin + (1.0f - y1) * plotH;
        dc->DrawLine(sx0, sy0, sx1, sy1, 2.0f, curveCol);
    }

    // Control points and handles
    for (size_t i = 0; i < points_.size(); ++i) {
        const auto& pt = points_[i];
        float px = margin + pt.x * plotW;
        float py = margin + (1.0f - pt.y) * plotH;

        // Handle lines
        Color4 handleCol{140, 140, 160, 150};
        if (i > 0) {
            float hix = margin + (pt.x + pt.handleInX) * plotW;
            float hiy = margin + (1.0f - (pt.y + pt.handleInY)) * plotH;
            dc->DrawLine(px, py, hix, hiy, 1.0f, handleCol);
            dc->DrawCircle(hix, hiy, 3.0f, handleCol);
        }
        if (i + 1 < points_.size()) {
            float hox = margin + (pt.x + pt.handleOutX) * plotW;
            float hoy = margin + (1.0f - (pt.y + pt.handleOutY)) * plotH;
            dc->DrawLine(px, py, hox, hoy, 1.0f, handleCol);
            dc->DrawCircle(hox, hoy, 3.0f, handleCol);
        }

        // Point
        Color4 ptCol = pt.selected ? Color4{255, 200, 60, 255}
                                   : Color4{220, 220, 240, 255};
        dc->DrawCircle(px, py, 5.0f, ptCol);
    }
}

} // namespace ui
} // namespace koilo
