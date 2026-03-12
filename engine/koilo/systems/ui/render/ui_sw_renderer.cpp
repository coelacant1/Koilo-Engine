// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/ui/render/ui_sw_renderer.hpp>
#include <algorithm>
#include <cstring>


namespace koilo {
namespace ui {

void UISWRenderer::Resize(int width, int height) {
    if (width == width_ && height == height_) return;
    width_  = width;
    height_ = height;
    pixels_.resize(width * height * 4, 0);
}

void UISWRenderer::Clear() {
    std::memset(pixels_.data(), 0, pixels_.size());
}

void UISWRenderer::Clear(Color4 color) {
    for (int i = 0; i < width_ * height_; ++i) {
        pixels_[i * 4 + 0] = color.r;
        pixels_[i * 4 + 1] = color.g;
        pixels_[i * 4 + 2] = color.b;
        pixels_[i * 4 + 3] = color.a;
    }
}

void UISWRenderer::Render(const UIDrawList& drawList,
                           const font::GlyphAtlas* atlas) {
    scissorX_ = 0;
    scissorY_ = 0;
    scissorW_ = width_;
    scissorH_ = height_;

    for (size_t i = 0; i < drawList.Size(); ++i) {
        const DrawCmd& cmd = drawList[i];

        switch (cmd.type) {
        case DrawCmdType::SolidRect:
            FillRect(static_cast<int>(cmd.x), static_cast<int>(cmd.y),
                     static_cast<int>(cmd.w), static_cast<int>(cmd.h),
                     cmd.color);
            break;

        case DrawCmdType::BorderRect:
            DrawBorder(static_cast<int>(cmd.x), static_cast<int>(cmd.y),
                       static_cast<int>(cmd.w), static_cast<int>(cmd.h),
                       static_cast<int>(cmd.borderWidth), cmd.color);
            break;

        case DrawCmdType::TexturedRect:
            if (atlas) {
                BlitAtlasRect(cmd, *atlas);
            }
            break;

        case DrawCmdType::RoundedRect: {
            float maxR = std::max({cmd.cornerRadius[0], cmd.cornerRadius[1],
                                   cmd.cornerRadius[2], cmd.cornerRadius[3]});
            FillRoundedRect(static_cast<int>(cmd.x), static_cast<int>(cmd.y),
                            static_cast<int>(cmd.w), static_cast<int>(cmd.h),
                            maxR, cmd.color);
            break;
        }

        case DrawCmdType::RoundedBorderRect: {
            float maxR = std::max({cmd.cornerRadius[0], cmd.cornerRadius[1],
                                   cmd.cornerRadius[2], cmd.cornerRadius[3]});
            DrawRoundedBorder(static_cast<int>(cmd.x), static_cast<int>(cmd.y),
                              static_cast<int>(cmd.w), static_cast<int>(cmd.h),
                              maxR, static_cast<int>(cmd.borderWidth),
                              cmd.color);
            break;
        }

        case DrawCmdType::PushScissor:
            scissorStack_.push_back({scissorX_, scissorY_,
                                     scissorW_, scissorH_});
            scissorX_ = cmd.scissorX;
            scissorY_ = cmd.scissorY;
            scissorW_ = cmd.scissorW;
            scissorH_ = cmd.scissorH;
            break;

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

void UISWRenderer::DrawBorder(int x, int y, int w, int h, int bw, Color4 color) {
    FillRect(x, y, w, bw, color);             // top
    FillRect(x, y + h - bw, w, bw, color);    // bottom
    FillRect(x, y + bw, bw, h - bw * 2, color); // left
    FillRect(x + w - bw, y + bw, bw, h - bw * 2, color); // right
}

void UISWRenderer::BlitAtlasRect(const DrawCmd& cmd, const font::GlyphAtlas& atlas) {
    int dstX = static_cast<int>(cmd.x);
    int dstY = static_cast<int>(cmd.y);
    int dstW = static_cast<int>(cmd.w);
    int dstH = static_cast<int>(cmd.h);

    int aw = atlas.Width();
    int srcX = static_cast<int>(cmd.u0 * aw);
    int srcY = static_cast<int>(cmd.v0 * atlas.Height());
    int srcW = static_cast<int>((cmd.u1 - cmd.u0) * aw);
    int srcH = static_cast<int>((cmd.v1 - cmd.v0) * atlas.Height());

    if (srcW <= 0 || srcH <= 0) return;
    const uint8_t* atlasPixels = atlas.Pixels();

    for (int py = 0; py < dstH; ++py) {
        int sy = srcY + py * srcH / dstH;
        for (int px = 0; px < dstW; ++px) {
            int sx = srcX + px * srcW / dstW;
            uint8_t alpha = atlasPixels[sy * aw + sx];
            if (alpha == 0) continue;
            Color4 c = cmd.color;
            c.a = static_cast<uint8_t>(
                (static_cast<int>(c.a) * alpha) / 255);
            BlendPixel(dstX + px, dstY + py, c);
        }
    }
}

bool UISWRenderer::IsInsideRoundedRect(int px, int py, int rx, int ry,
                                        int rw, int rh, float radius) {
    float r = radius;
    // Check four corner circles
    float dx = 0.0f, dy = 0.0f;
    if (px < rx + r && py < ry + r) {
        // Top-left corner
        dx = (rx + r) - px; dy = (ry + r) - py;
    } else if (px >= rx + rw - r && py < ry + r) {
        // Top-right corner
        dx = px - (rx + rw - r); dy = (ry + r) - py;
    } else if (px < rx + r && py >= ry + rh - r) {
        // Bottom-left corner
        dx = (rx + r) - px; dy = py - (ry + rh - r);
    } else if (px >= rx + rw - r && py >= ry + rh - r) {
        // Bottom-right corner
        dx = px - (rx + rw - r); dy = py - (ry + rh - r);
    } else {
        return true; // Not in a corner region
    }
    return (dx * dx + dy * dy) <= (r * r);
}

void UISWRenderer::FillRoundedRect(int x, int y, int w, int h,
                                    float radius, Color4 color) {
    int x0 = std::max(x, std::max(0, scissorX_));
    int y0 = std::max(y, std::max(0, scissorY_));
    int x1 = std::min(x + w, std::min(width_, scissorX_ + scissorW_));
    int y1 = std::min(y + h, std::min(height_, scissorY_ + scissorH_));

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            if (IsInsideRoundedRect(px, py, x, y, w, h, radius)) {
                BlendPixel(px, py, color);
            }
        }
    }
}

void UISWRenderer::DrawRoundedBorder(int x, int y, int w, int h,
                                      float radius, int bw, Color4 color) {
    int x0 = std::max(x, std::max(0, scissorX_));
    int y0 = std::max(y, std::max(0, scissorY_));
    int x1 = std::min(x + w, std::min(width_, scissorX_ + scissorW_));
    int y1 = std::min(y + h, std::min(height_, scissorY_ + scissorH_));

    float innerR = std::max(radius - bw, 0.0f);
    int ix = x + bw, iy = y + bw, iw = w - bw * 2, ih = h - bw * 2;

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            bool inOuter = IsInsideRoundedRect(px, py, x, y, w, h, radius);
            bool inInner = (iw > 0 && ih > 0) &&
                           IsInsideRoundedRect(px, py, ix, iy, iw, ih, innerR);
            if (inOuter && !inInner) {
                BlendPixel(px, py, color);
            }
        }
    }
}

} // namespace ui
} // namespace koilo
