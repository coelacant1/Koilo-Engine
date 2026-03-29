// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_sw_renderer.cpp
 * @brief CPU software UI renderer implementation.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include <koilo/systems/ui/render/ui_sw_renderer.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>


namespace koilo {
namespace ui {

// ============================================================================
// Public methods
// ============================================================================

// Resize the pixel buffer
void UISWRenderer::Resize(int width, int height) {
    if (width == width_ && height == height_) return;
    width_  = width;
    height_ = height;
    pixels_.resize(width * height * 4, 0);
}

// Clear the pixel buffer to transparent
void UISWRenderer::Clear() {
    std::memset(pixels_.data(), 0, pixels_.size());
}

// Clear the pixel buffer to a specific color
void UISWRenderer::Clear(Color4 color) {
    for (int i = 0; i < width_ * height_; ++i) {
        pixels_[i * 4 + 0] = color.r;
        pixels_[i * 4 + 1] = color.g;
        pixels_[i * 4 + 2] = color.b;
        pixels_[i * 4 + 3] = color.a;
    }
}

// Render a draw list to the pixel buffer
void UISWRenderer::RenderDirect(const UIDrawList& drawList,
                           const font::GlyphAtlas* atlas,
                           const font::GlyphAtlas* boldAtlas) {
    scissorX_ = 0;
    scissorY_ = 0;
    scissorW_ = width_;
    scissorH_ = height_;

    for (size_t i = 0; i < drawList.Size(); ++i) {
        const DrawCmd& cmd = drawList[i];

        switch (cmd.type) {
        case DrawCmdType::SolidRect:
            FillRect(static_cast<int>(std::round(cmd.x)),
                     static_cast<int>(std::round(cmd.y)),
                     static_cast<int>(std::round(cmd.w)),
                     static_cast<int>(std::round(cmd.h)),
                     cmd.color);
            break;

        case DrawCmdType::BorderRect:
            DrawBorder(static_cast<int>(std::round(cmd.x)),
                       static_cast<int>(std::round(cmd.y)),
                       static_cast<int>(std::round(cmd.w)),
                       static_cast<int>(std::round(cmd.h)),
                       std::max(1, static_cast<int>(std::round(cmd.borderWidth))),
                       cmd.color);
            break;

        case DrawCmdType::TexturedRect: {
            // Pick the correct atlas based on texture handle sentinel
            const font::GlyphAtlas* activeAtlas = nullptr;
            if (cmd.textureHandle == 2 && boldAtlas) {
                activeAtlas = boldAtlas;
            } else if (atlas) {
                activeAtlas = atlas;
            }
            if (activeAtlas) {
                BlitAtlasRect(cmd, *activeAtlas);
            }
            break;
        }

        case DrawCmdType::RoundedRect:
            FillRoundedRect(static_cast<int>(std::round(cmd.x)),
                            static_cast<int>(std::round(cmd.y)),
                            static_cast<int>(std::round(cmd.w)),
                            static_cast<int>(std::round(cmd.h)),
                            cmd.cornerRadius, cmd.color);
            break;

        case DrawCmdType::RoundedBorderRect:
            DrawRoundedBorder(static_cast<int>(std::round(cmd.x)),
                              static_cast<int>(std::round(cmd.y)),
                              static_cast<int>(std::round(cmd.w)),
                              static_cast<int>(std::round(cmd.h)),
                              cmd.cornerRadius,
                              std::max(1, static_cast<int>(std::round(cmd.borderWidth))),
                              cmd.color);
            break;

        case DrawCmdType::Line:
            DrawLine(cmd.x, cmd.y, cmd.w, cmd.h, cmd.borderWidth, cmd.color);
            break;

        case DrawCmdType::FilledCircle:
            DrawFilledCircle(cmd.x, cmd.y, cmd.w, cmd.color);
            break;

        case DrawCmdType::CircleOutline:
            DrawCircleOutline(cmd.x, cmd.y, cmd.w, cmd.borderWidth, cmd.color);
            break;

        case DrawCmdType::Triangle:
            DrawTriangle(cmd.x, cmd.y, cmd.w, cmd.h, cmd.x2, cmd.y2, cmd.color);
            break;

        case DrawCmdType::PushScissor: {
            scissorStack_.push_back({scissorX_, scissorY_,
                                     scissorW_, scissorH_});
            int sx = cmd.scissorX;
            int sy = cmd.scissorY;
            int sw = cmd.scissorW;
            int sh = cmd.scissorH;
            // Intersect with current scissor (matches GPU behavior)
            int x1 = std::max(sx, scissorX_);
            int y1 = std::max(sy, scissorY_);
            int x2 = std::min(sx + sw, scissorX_ + scissorW_);
            int y2 = std::min(sy + sh, scissorY_ + scissorH_);
            scissorX_ = x1;
            scissorY_ = y1;
            scissorW_ = std::max(0, x2 - x1);
            scissorH_ = std::max(0, y2 - y1);
            break;
        }

        case DrawCmdType::PopScissor:
            if (!scissorStack_.empty()) {
                auto& s = scissorStack_.back();
                scissorX_ = s.x; scissorY_ = s.y;
                scissorW_ = s.w; scissorH_ = s.h;
                scissorStack_.pop_back();
            } else {
                scissorX_ = 0; scissorY_ = 0;
                scissorW_ = width_; scissorH_ = height_;
            }
            break;
        }
    }
}

// ============================================================================
// Private rasterization helpers
// ============================================================================

// Blend a single pixel with alpha compositing
void UISWRenderer::BlendPixel(int x, int y, Color4 c) {
    if (x < scissorX_ || x >= scissorX_ + scissorW_ ||
        y < scissorY_ || y >= scissorY_ + scissorH_ ||
        x < 0 || x >= width_ || y < 0 || y >= height_) return;

    int idx = (y * width_ + x) * 4;
    if (c.a == 255) {
        pixels_[idx + 0] = c.r;
        pixels_[idx + 1] = c.g;
        pixels_[idx + 2] = c.b;
        pixels_[idx + 3] = 255;
    } else if (c.a > 0) {
        uint8_t invA = 255 - c.a;
        pixels_[idx + 0] = static_cast<uint8_t>(
            (c.r * c.a + pixels_[idx + 0] * invA) / 255);
        pixels_[idx + 1] = static_cast<uint8_t>(
            (c.g * c.a + pixels_[idx + 1] * invA) / 255);
        pixels_[idx + 2] = static_cast<uint8_t>(
            (c.b * c.a + pixels_[idx + 2] * invA) / 255);
        pixels_[idx + 3] = static_cast<uint8_t>(
            std::min(255, pixels_[idx + 3] + c.a));
    }
}

// Fill a clipped axis-aligned rectangle
void UISWRenderer::FillRect(int x, int y, int w, int h, Color4 color) {
    int x0 = std::max(x, std::max(0, scissorX_));
    int y0 = std::max(y, std::max(0, scissorY_));
    int x1 = std::min(x + w, std::min(width_, scissorX_ + scissorW_));
    int y1 = std::min(y + h, std::min(height_, scissorY_ + scissorH_));

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            BlendPixel(px, py, color);
        }
    }
}

// Draw a rectangular border (four edge rects)
void UISWRenderer::DrawBorder(int x, int y, int w, int h, int bw, Color4 color) {
    FillRect(x, y, w, bw, color);             // top
    FillRect(x, y + h - bw, w, bw, color);    // bottom
    FillRect(x, y + bw, bw, h - bw * 2, color); // left
    FillRect(x + w - bw, y + bw, bw, h - bw * 2, color); // right
}

// Blit a glyph atlas region with alpha blending
void UISWRenderer::BlitAtlasRect(const DrawCmd& cmd, const font::GlyphAtlas& atlas) {
    int dstX = static_cast<int>(std::round(cmd.x));
    int dstY = static_cast<int>(std::round(cmd.y));
    int dstW = static_cast<int>(std::round(cmd.w));
    int dstH = static_cast<int>(std::round(cmd.h));

    int aw = atlas.Width();
    int ah = atlas.Height();
    float srcXf = cmd.u0 * aw;
    float srcYf = cmd.v0 * ah;
    float srcWf = (cmd.u1 - cmd.u0) * aw;
    float srcHf = (cmd.v1 - cmd.v0) * ah;

    if (srcWf <= 0.0f || srcHf <= 0.0f || dstW <= 0 || dstH <= 0) return;
    const uint8_t* atlasPixels = atlas.Pixels();

    for (int py = 0; py < dstH; ++py) {
        int sy = static_cast<int>(srcYf + (py + 0.5f) * srcHf / dstH);
        if (sy < 0 || sy >= ah) continue;
        for (int px = 0; px < dstW; ++px) {
            int sx = static_cast<int>(srcXf + (px + 0.5f) * srcWf / dstW);
            if (sx < 0 || sx >= aw) continue;
            uint8_t alpha = atlasPixels[sy * aw + sx];
            if (alpha == 0) continue;
            Color4 c = cmd.color;
            c.a = static_cast<uint8_t>(
                (static_cast<int>(c.a) * alpha) / 255);
            BlendPixel(dstX + px, dstY + py, c);
        }
    }
}

// Compute coverage for a pixel inside a rounded rectangle (0.0 = outside, 1.0 = inside)
// Takes per-corner radii: TL, TR, BR, BL
float UISWRenderer::RoundedRectCoverage(int px, int py, int rx, int ry,
                                         int rw, int rh,
                                         float rTL, float rTR,
                                         float rBR, float rBL) {
    // Bounds check: pixel outside the rectangle is 0 coverage
    if (px < rx || px >= rx + rw || py < ry || py >= ry + rh) {
        return 0.0f;
    }

    float dx = 0.0f, dy = 0.0f;
    float r;
    if (px < rx + rTL && py < ry + rTL) {
        r = rTL;
        dx = (rx + r) - (px + 0.5f); dy = (ry + r) - (py + 0.5f);
    } else if (px >= rx + rw - rTR && py < ry + rTR) {
        r = rTR;
        dx = (px + 0.5f) - (rx + rw - r); dy = (ry + r) - (py + 0.5f);
    } else if (px < rx + rBL && py >= ry + rh - rBL) {
        r = rBL;
        dx = (rx + r) - (px + 0.5f); dy = (py + 0.5f) - (ry + rh - r);
    } else if (px >= rx + rw - rBR && py >= ry + rh - rBR) {
        r = rBR;
        dx = (px + 0.5f) - (rx + rw - r); dy = (py + 0.5f) - (ry + rh - r);
    } else {
        return 1.0f;
    }
    float dist = std::sqrt(dx * dx + dy * dy);
    // smoothstep AA matching GPU SDF: smoothstep(-0.75, 0.75, r - dist)
    float edge = r - dist;
    if (edge >= 0.75f) return 1.0f;
    if (edge <= -0.75f) return 0.0f;
    float t = (edge + 0.75f) / 1.5f;
    return t * t * (3.0f - 2.0f * t);
}

// Fill a rounded rectangle with anti-aliased corners (per-corner radii)
void UISWRenderer::FillRoundedRect(int x, int y, int w, int h,
                                    const float radii[4], Color4 color) {
    int x0 = std::max(x, std::max(0, scissorX_));
    int y0 = std::max(y, std::max(0, scissorY_));
    int x1 = std::min(x + w, std::min(width_, scissorX_ + scissorW_));
    int y1 = std::min(y + h, std::min(height_, scissorY_ + scissorH_));

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            float cov = RoundedRectCoverage(px, py, x, y, w, h,
                                            radii[0], radii[1],
                                            radii[2], radii[3]);
            if (cov > 0.0f) {
                Color4 c = color;
                c.a = static_cast<uint8_t>(c.a * cov);
                BlendPixel(px, py, c);
            }
        }
    }
}

// Draw a rounded border (outer minus inner rounded rect) with AA
void UISWRenderer::DrawRoundedBorder(int x, int y, int w, int h,
                                      const float radii[4], int bw,
                                      Color4 color) {
    int x0 = std::max(x, std::max(0, scissorX_));
    int y0 = std::max(y, std::max(0, scissorY_));
    int x1 = std::min(x + w, std::min(width_, scissorX_ + scissorW_));
    int y1 = std::min(y + h, std::min(height_, scissorY_ + scissorH_));

    float innerR[4] = {
        std::max(radii[0] - bw, 0.0f),
        std::max(radii[1] - bw, 0.0f),
        std::max(radii[2] - bw, 0.0f),
        std::max(radii[3] - bw, 0.0f)
    };
    int ix = x + bw, iy = y + bw, iw = w - bw * 2, ih = h - bw * 2;

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            float outer = RoundedRectCoverage(px, py, x, y, w, h,
                                              radii[0], radii[1],
                                              radii[2], radii[3]);
            float inner = (iw > 0 && ih > 0)
                ? RoundedRectCoverage(px, py, ix, iy, iw, ih,
                                      innerR[0], innerR[1],
                                      innerR[2], innerR[3])
                : 0.0f;
            float cov = outer - inner;
            if (cov > 0.0f) {
                Color4 c = color;
                c.a = static_cast<uint8_t>(c.a * std::min(1.0f, cov));
                BlendPixel(px, py, c);
            }
        }
    }
}

// Draw a line segment with width using Bresenham + perpendicular thickness
void UISWRenderer::DrawLine(float x0, float y0, float x1, float y1,
                             float width, Color4 color) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) return;

    // Perpendicular direction, scaled to half-width
    float hw = width * 0.5f;
    float nx = -dy / len * hw;
    float ny =  dx / len * hw;

    // Build a quad from the line endpoints expanded by half-width
    float qx[4] = { x0 + nx, x0 - nx, x1 - nx, x1 + nx };
    float qy[4] = { y0 + ny, y0 - ny, y1 - ny, y1 + ny };

    // Rasterize as two triangles
    DrawTriangle(qx[0], qy[0], qx[1], qy[1], qx[2], qy[2], color);
    DrawTriangle(qx[0], qy[0], qx[2], qy[2], qx[3], qy[3], color);
}

// Draw a filled circle using per-pixel distance test
void UISWRenderer::DrawFilledCircle(float cx, float cy, float radius,
                                     Color4 color) {
    int x0 = std::max(static_cast<int>(cx - radius), std::max(0, scissorX_));
    int y0 = std::max(static_cast<int>(cy - radius), std::max(0, scissorY_));
    int x1 = std::min(static_cast<int>(cx + radius + 1), std::min(width_, scissorX_ + scissorW_));
    int y1 = std::min(static_cast<int>(cy + radius + 1), std::min(height_, scissorY_ + scissorH_));

    float r2 = radius * radius;
    for (int py = y0; py < y1; ++py) {
        float dy = py + 0.5f - cy;
        for (int px = x0; px < x1; ++px) {
            float ddx = px + 0.5f - cx;
            float d2 = ddx * ddx + dy * dy;
            if (d2 <= (radius + 1.0f) * (radius + 1.0f)) {
                float dist = std::sqrt(d2);
                // smoothstep AA matching GPU SDF
                float edge = radius - dist;
                float aa;
                if (edge >= 0.75f) aa = 1.0f;
                else if (edge <= -0.75f) aa = 0.0f;
                else { float t = (edge + 0.75f) / 1.5f; aa = t * t * (3.0f - 2.0f * t); }
                if (aa > 0.0f) {
                    Color4 c = color;
                    c.a = static_cast<uint8_t>(c.a * aa);
                    BlendPixel(px, py, c);
                }
            }
        }
    }
}

// Draw a circle outline
void UISWRenderer::DrawCircleOutline(float cx, float cy, float radius,
                                      float lineWidth, Color4 color) {
    float hw = lineWidth * 0.5f;
    float rOuter = radius + hw;
    float rInner = radius - hw;

    int x0 = std::max(static_cast<int>(cx - rOuter), std::max(0, scissorX_));
    int y0 = std::max(static_cast<int>(cy - rOuter), std::max(0, scissorY_));
    int x1 = std::min(static_cast<int>(cx + rOuter + 1), std::min(width_, scissorX_ + scissorW_));
    int y1 = std::min(static_cast<int>(cy + rOuter + 1), std::min(height_, scissorY_ + scissorH_));

    float rO2 = rOuter * rOuter;
    for (int py = y0; py < y1; ++py) {
        float dy = py + 0.5f - cy;
        for (int px = x0; px < x1; ++px) {
            float ddx = px + 0.5f - cx;
            float d2 = ddx * ddx + dy * dy;
            if (d2 <= (rOuter + 1.0f) * (rOuter + 1.0f)) {
                float dist = std::sqrt(d2);
                // smoothstep AA on both edges
                float outerEdge = rOuter - dist;
                float innerEdge = dist - rInner;
                float aaOuter, aaInner;
                if (outerEdge >= 0.75f) aaOuter = 1.0f;
                else if (outerEdge <= -0.75f) aaOuter = 0.0f;
                else { float t = (outerEdge + 0.75f) / 1.5f; aaOuter = t * t * (3.0f - 2.0f * t); }
                if (innerEdge >= 0.75f) aaInner = 1.0f;
                else if (innerEdge <= -0.75f) aaInner = 0.0f;
                else { float t = (innerEdge + 0.75f) / 1.5f; aaInner = t * t * (3.0f - 2.0f * t); }
                float aa = aaOuter * aaInner;
                if (aa > 0.0f) {
                    Color4 c = color;
                    c.a = static_cast<uint8_t>(c.a * aa);
                    BlendPixel(px, py, c);
                }
            }
        }
    }
}

// Draw a filled triangle using barycentric coordinates
void UISWRenderer::DrawTriangle(float x0, float y0, float x1, float y1,
                                 float x2, float y2, Color4 color) {
    // Bounding box
    int minX = std::max(static_cast<int>(std::min({x0, x1, x2})), std::max(0, scissorX_));
    int minY = std::max(static_cast<int>(std::min({y0, y1, y2})), std::max(0, scissorY_));
    int maxX = std::min(static_cast<int>(std::max({x0, x1, x2}) + 1), std::min(width_, scissorX_ + scissorW_));
    int maxY = std::min(static_cast<int>(std::max({y0, y1, y2}) + 1), std::min(height_, scissorY_ + scissorH_));

    // Edge function approach
    float denom = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
    if (std::abs(denom) < 0.001f) return; // degenerate
    float invDenom = 1.0f / denom;

    for (int py = minY; py < maxY; ++py) {
        float ppy = py + 0.5f;
        for (int px = minX; px < maxX; ++px) {
            float ppx = px + 0.5f;
            float w0 = ((y1 - y2) * (ppx - x2) + (x2 - x1) * (ppy - y2)) * invDenom;
            float w1 = ((y2 - y0) * (ppx - x2) + (x0 - x2) * (ppy - y2)) * invDenom;
            float w2 = 1.0f - w0 - w1;
            if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {
                BlendPixel(px, py, color);
            }
        }
    }
}

// ============================================================================
// IUIRenderer interface implementation
// ============================================================================

void UISWRenderer::Shutdown() {
    pixels_.clear();
    width_ = 0;
    height_ = 0;
    font_ = nullptr;
    boldFont_ = nullptr;
}

uint32_t UISWRenderer::SetFont(font::Font* font) {
    font_ = font;
    return font ? 1u : 0u;
}

uint32_t UISWRenderer::SetBoldFont(font::Font* font) {
    boldFont_ = font;
    return font ? 2u : 0u;
}

void UISWRenderer::SyncFontAtlases(font::Font* font, uint32_t& /*fontHandle*/,
                                    font::Font* boldFont, uint32_t& /*boldHandle*/) {
    // SW renderer reads atlas data directly - just clear dirty flags
    if (font && font->Atlas().IsDirty())
        font->Atlas().ClearDirty();
    if (boldFont && boldFont->Atlas().IsDirty())
        boldFont->Atlas().ClearDirty();
}

void UISWRenderer::Render(const UIDrawList& drawList,
                           int viewportW, int viewportH) {
    Resize(viewportW, viewportH);
    Clear(Color4{30, 30, 30, 255});

    const font::GlyphAtlas* atlas = (font_ && font_->IsLoaded())
        ? &font_->Atlas() : nullptr;
    const font::GlyphAtlas* boldAtlas = (boldFont_ && boldFont_->IsLoaded())
        ? &boldFont_->Atlas() : nullptr;

    RenderDirect(drawList, atlas, boldAtlas);
}

} // namespace ui
} // namespace koilo
