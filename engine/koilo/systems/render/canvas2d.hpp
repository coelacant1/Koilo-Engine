// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <koilo/core/color/color888.hpp>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <string>

namespace koilo {

/// @brief Immediate-mode 2D pixel canvas for script-driven rendering.
///
/// User-created resource. Scripts create a Canvas2D, draw onto it, and
/// call Attach() to register it for compositing during RenderFrame().
/// Multiple canvases can be attached simultaneously; they composite in
/// the order they were attached.
class Canvas2D {
public:
    Canvas2D() = default;

    ~Canvas2D() {
        Detach(); // ensure removal from active list on destruction
    }

    // Non-copyable (tracked by pointer in active list)
    Canvas2D(const Canvas2D&) = delete;
    Canvas2D& operator=(const Canvas2D&) = delete;

    /// @brief Register this canvas for compositing during RenderFrame().
    void Attach() {
        if (!attached_) {
            attached_ = true;
            ActiveList().push_back(this);
        }
    }

    /// @brief Unregister this canvas from compositing.
    void Detach() {
        if (attached_) {
            attached_ = false;
            auto& list = ActiveList();
            list.erase(std::remove(list.begin(), list.end(), this), list.end());
        }
    }

    bool IsAttached() const { return attached_; }

    /// @brief Resize the canvas buffer.
    void Resize(int w, int h) {
        width_  = w;
        height_ = h;
        rgba_.assign(static_cast<size_t>(w) * h, 0u);
        dirtyMinX_ = w; dirtyMinY_ = h;
        dirtyMaxX_ = -1; dirtyMaxY_ = -1;
        ++modVersion_;
    }

    /// @brief Ensure the canvas is at least the given size. Only resizes if needed.
    void EnsureSize(int w, int h) {
        if (width_ != w || height_ != h) Resize(w, h);
    }

    /// @brief Clear all drawn pixels (make transparent).
    void Clear() {
        // Only clear the dirty region, not the whole buffer.
        if (dirtyMaxX_ >= 0) {
            const int rowSpan = dirtyMaxX_ - dirtyMinX_ + 1;
            for (int y = dirtyMinY_; y <= dirtyMaxY_; ++y) {
                std::fill_n(rgba_.data() + y * width_ + dirtyMinX_, rowSpan, 0u);
            }
            ++modVersion_;
        }
        dirtyMinX_ = width_; dirtyMinY_ = height_;
        dirtyMaxX_ = -1; dirtyMaxY_ = -1;
    }

    /// @brief Fill the entire canvas with a solid color.
    void ClearWithColor(uint8_t r, uint8_t g, uint8_t b) {
        const uint32_t packed = PackRGBA(r, g, b, 255);
        std::fill(rgba_.begin(), rgba_.end(), packed);
        dirtyMinX_ = 0; dirtyMinY_ = 0;
        dirtyMaxX_ = width_ - 1; dirtyMaxY_ = height_ - 1;
        ++modVersion_;
    }

    /// @brief Set a single pixel.
    void SetPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            rgba_[y * width_ + x] = PackRGBA(r, g, b, 255);
            if (x < dirtyMinX_) dirtyMinX_ = x;
            if (x > dirtyMaxX_) dirtyMaxX_ = x;
            if (y < dirtyMinY_) dirtyMinY_ = y;
            if (y > dirtyMaxY_) dirtyMaxY_ = y;
            ++modVersion_;
        }
    }

    /// @brief Fill a rectangle with a solid color.
    void FillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
        // Clip to canvas bounds and use a packed-uint32_t row fill - far
        // faster than the per-pixel bounds check + dirty-rect bump that
        // SetPixel does for each of (w*h) pixels.
        const int x0 = std::max(0, x);
        const int y0 = std::max(0, y);
        const int x1 = std::min(width_,  x + w);
        const int y1 = std::min(height_, y + h);
        if (x1 <= x0 || y1 <= y0) return;
        const uint32_t packed = PackRGBA(r, g, b, 255);
        const int span = x1 - x0;
        for (int yy = y0; yy < y1; ++yy) {
            std::fill_n(rgba_.data() + yy * width_ + x0, span, packed);
        }
        ExpandDirty(x0, y0, x1 - 1, y1 - 1);
    }

    /// @brief Draw a rectangle outline.
    void DrawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
        for (int dx = 0; dx < w; ++dx) {
            WritePixel(x + dx, y,         r, g, b);
            WritePixel(x + dx, y + h - 1, r, g, b);
        }
        for (int dy = 0; dy < h; ++dy) {
            WritePixel(x,         y + dy, r, g, b);
            WritePixel(x + w - 1, y + dy, r, g, b);
        }
        ExpandDirty(std::max(0, x), std::max(0, y),
                    std::min(width_  - 1, x + w - 1),
                    std::min(height_ - 1, y + h - 1));
    }

    /// @brief Draw a line using Bresenham's algorithm.
    void DrawLine(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b) {
        int ddx = x1 - x0; int dx = ddx < 0 ? -ddx : ddx;
        int ddy = y1 - y0; int dy = ddy < 0 ?  ddy : -ddy;
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        const int bx0 = std::max(0, std::min(x0, x1));
        const int by0 = std::max(0, std::min(y0, y1));
        const int bx1 = std::min(width_  - 1, std::max(x0, x1));
        const int by1 = std::min(height_ - 1, std::max(y0, y1));
        while (true) {
            WritePixel(x0, y0, r, g, b);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
        if (bx1 >= bx0 && by1 >= by0) ExpandDirty(bx0, by0, bx1, by1);
    }

    /// @brief Draw a circle outline using the midpoint algorithm.
    void DrawCircle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b) {
        int x = radius, y = 0, err = 1 - radius;
        while (x >= y) {
            WritePixel(cx+x, cy+y, r, g, b); WritePixel(cx-x, cy+y, r, g, b);
            WritePixel(cx+x, cy-y, r, g, b); WritePixel(cx-x, cy-y, r, g, b);
            WritePixel(cx+y, cy+x, r, g, b); WritePixel(cx-y, cy+x, r, g, b);
            WritePixel(cx+y, cy-x, r, g, b); WritePixel(cx-y, cy-x, r, g, b);
            y++;
            if (err < 0) { err += 2*y + 1; }
            else { x--; err += 2*(y - x) + 1; }
        }
        ExpandDirty(std::max(0, cx - radius), std::max(0, cy - radius),
                    std::min(width_  - 1, cx + radius),
                    std::min(height_ - 1, cy + radius));
    }

    /// @brief Draw a filled circle.
    void FillCircle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b) {
        // Span-fill: one row at a time, clipped to canvas, bookkeeping
        // amortized across the whole circle.
        const uint32_t packed = PackRGBA(r, g, b, 255);
        const int r2 = radius * radius;
        const int y0 = std::max(0, cy - radius);
        const int y1 = std::min(height_ - 1, cy + radius);
        if (y0 > y1) return;
        for (int yy = y0; yy <= y1; ++yy) {
            const int dy = yy - cy;
            const int dxMax2 = r2 - dy * dy;
            if (dxMax2 < 0) continue;
            // integer sqrt - radius is small (UI-scale) so this is cheap
            int dxMax = 0;
            while ((dxMax + 1) * (dxMax + 1) <= dxMax2) ++dxMax;
            const int x0 = std::max(0, cx - dxMax);
            const int x1 = std::min(width_ - 1, cx + dxMax);
            if (x1 < x0) continue;
            std::fill_n(rgba_.data() + yy * width_ + x0,
                        static_cast<size_t>(x1 - x0 + 1), packed);
        }
        ExpandDirty(std::max(0, cx - radius), y0,
                    std::min(width_ - 1, cx + radius), y1);
    }

    /// @brief Resize the canvas pixel buffer (alias for Resize).
    void RenderSize(int w, int h) { Resize(w, h); }

    /// @brief Set the display scale: canvas pixels are scaled up to this area
    ///        when composited onto the framebuffer (nearest-neighbour).
    ///        Defaults to the render size if never called.
    void Scale(int w, int h) { displayW_ = w; displayH_ = h; }
    void DrawText(int x, int y, const std::string& text,
                  uint8_t r, uint8_t g, uint8_t b) {
        if (text.empty() || width_ <= 0 || height_ <= 0) return;
        const uint32_t packed = PackRGBA(r, g, b, 255);
        const int textPxW = static_cast<int>(text.size()) * 4 - 1; // 3px + 1px gap, last char no gap
        // Compute clipped bounding box once for dirty-rect bookkeeping.
        const int bx0 = std::max(0, x);
        const int by0 = std::max(0, y);
        const int bx1 = std::min(width_  - 1, x + textPxW - 1);
        const int by1 = std::min(height_ - 1, y + 5      - 1);
        const bool anyVisible = (bx1 >= bx0 && by1 >= by0);

        // Fast path: when the entire text bbox is fully inside the canvas
        // we can write each glyph pixel without a per-pixel bounds check.
        const bool fullyInside = (x >= 0 && y >= 0
                               && x + textPxW <= width_
                               && y + 5      <= height_);
        if (fullyInside) {
            for (size_t ci = 0; ci < text.size(); ++ci) {
                const uint8_t* glyph = GetTinyGlyph(text[ci]);
                if (!glyph) continue;
                const int cx = x + static_cast<int>(ci) * 4;
                for (int row = 0; row < 5; ++row) {
                    const uint8_t bits = glyph[row];
                    if (!bits) continue;
                    uint32_t* dst = rgba_.data() + (y + row) * width_ + cx;
                    if (bits & 0x4) dst[0] = packed;
                    if (bits & 0x2) dst[1] = packed;
                    if (bits & 0x1) dst[2] = packed;
                }
            }
        } else {
            for (size_t ci = 0; ci < text.size(); ++ci) {
                const uint8_t* glyph = GetTinyGlyph(text[ci]);
                if (!glyph) continue;
                const int cx = x + static_cast<int>(ci) * 4;
                for (int row = 0; row < 5; ++row) {
                    const uint8_t bits = glyph[row];
                    if (!bits) continue;
                    for (int col = 0; col < 3; ++col) {
                        if (bits & (0x4 >> col)) WritePixel(cx + col, y + row, r, g, b);
                    }
                }
            }
        }
        if (anyVisible) ExpandDirty(bx0, by0, bx1, by1);
    }

    /// @brief Composite drawn pixels over an existing framebuffer.
    ///        Only processes the dirty region; skips entirely if nothing was drawn.
    void CompositeOnto(Color888* buf, int bw, int bh) const {
        if (dirtyMaxX_ < 0) return;  // Nothing drawn since last clear

        int cw = width_;
        int ch = height_;
        int dw = displayW_ > 0 ? displayW_ : cw;
        int dh = displayH_ > 0 ? displayH_ : ch;

        // Map dirty rect from canvas coords to display coords
        int dMinX = dirtyMinX_ * dw / cw;
        int dMinY = dirtyMinY_ * dh / ch;
        int dMaxX = (dirtyMaxX_ + 1) * dw / cw;
        int dMaxY = (dirtyMaxY_ + 1) * dh / ch;

        int maxX = std::min(dMaxX, std::min(dw, bw));
        int maxY = std::min(dMaxY, std::min(dh, bh));
        int minX = std::max(0, dMinX);
        int minY = std::max(0, dMinY);

        for (int y = minY; y < maxY; ++y) {
            int sy = y * ch / dh;
            for (int x = minX; x < maxX; ++x) {
                int sx = x * cw / dw;
                uint32_t p = rgba_[sy * cw + sx];
                if ((p >> 24) & 0xFFu) {
                    buf[y * bw + x] = Color888(
                        static_cast<uint8_t>( p        & 0xFFu),
                        static_cast<uint8_t>((p >> 8 ) & 0xFFu),
                        static_cast<uint8_t>((p >> 16) & 0xFFu));
                }
            }
        }
    }

    int GetWidth()  const { return width_; }
    int GetHeight() const { return height_; }

    /// @brief GPU-ready packed RGBA8 pixel buffer.
    ///        Layout in memory: [R][G][B][A] per pixel, tightly packed,
    ///        rows in scanline order.  This matches VK_FORMAT_R8G8B8A8_UNORM
    ///        so the upload path can `memcpy` (or vkCmdCopyBufferToImage)
    ///        without per-pixel format conversion.
    const uint8_t* GetRGBA8() const {
        return reinterpret_cast<const uint8_t*>(rgba_.data());
    }
    /// @brief Size in bytes of the packed RGBA8 buffer (rows tightly packed).
    size_t GetRGBA8Bytes() const { return rgba_.size() * sizeof(uint32_t); }
    int GetDirtyMinX() const { return dirtyMinX_; }
    int GetDirtyMinY() const { return dirtyMinY_; }
    int GetDirtyMaxX() const { return dirtyMaxX_; }
    int GetDirtyMaxY() const { return dirtyMaxY_; }
    bool IsDirty() const { return dirtyMaxX_ >= 0; }
    /// Monotonic counter; bumps on any pixel mutation. Used by the GPU
    /// upload path to detect "no change since last upload" without
    /// hashing pixel data.
    uint64_t GetModVersion() const { return modVersion_; }

    // -- Static canvas management --------------------------------------

    /// @brief Get all currently attached canvases (compositing order).
    static std::vector<Canvas2D*>& ActiveList() {
        static std::vector<Canvas2D*> list;
        return list;
    }

    /// @brief Composite all attached canvases onto a framebuffer, then clear them.
    static void CompositeAll(Color888* buf, int bw, int bh) {
        for (auto* c : ActiveList()) {
            if (c->width_ > 0) {
                c->CompositeOnto(buf, bw, bh);
                c->Clear();
            }
        }
    }

    /// @brief Detach all canvases (call on engine reset/reload).
    static void DetachAll() {
        for (auto* c : ActiveList()) c->attached_ = false;
        ActiveList().clear();
    }

private:
    /// @brief Pack 4 bytes into a uint32_t in [R][G][B][A] memory order.
    ///        On little-endian (every platform we target) this is the
    ///        same as `(a<<24)|(b<<16)|(g<<8)|r`.
    static constexpr uint32_t PackRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        return  static_cast<uint32_t>(r)
             | (static_cast<uint32_t>(g) << 8)
             | (static_cast<uint32_t>(b) << 16)
             | (static_cast<uint32_t>(a) << 24);
    }

    /// @brief Bounds-checked single-pixel write.  Does NOT update dirty
    ///        rect or modVersion - callers MUST batch ExpandDirty() once
    ///        per logical drawing op.
    void WritePixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x >= 0 && x < width_ && y >= 0 && y < height_) {
            rgba_[y * width_ + x] = PackRGBA(r, g, b, 255);
        }
    }

    /// @brief Union the rect [x0..x1, y0..y1] (inclusive, already clipped
    ///        to canvas bounds) into the dirty rect, and bump modVersion.
    void ExpandDirty(int x0, int y0, int x1, int y1) {
        if (x0 < dirtyMinX_) dirtyMinX_ = x0;
        if (y0 < dirtyMinY_) dirtyMinY_ = y0;
        if (x1 > dirtyMaxX_) dirtyMaxX_ = x1;
        if (y1 > dirtyMaxY_) dirtyMaxY_ = y1;
        ++modVersion_;
    }

    std::vector<uint32_t> rgba_;  // packed RGBA8, row-major
    int width_    = 0;
    int height_   = 0;
    int displayW_ = 0;
    int displayH_ = 0;
    int dirtyMinX_ = 0;
    int dirtyMinY_ = 0;
    int dirtyMaxX_ = -1;
    int dirtyMaxY_ = -1;
    bool attached_ = false;
    uint64_t modVersion_ = 0;

    /// @brief 3x5 bitmap font glyph lookup. Returns 5-byte array (rows),
    ///        each byte uses lower 3 bits (bit2=left, bit0=right).
    static const uint8_t* GetTinyGlyph(char ch) {
        // Digits 0-9
        static const uint8_t digits[10][5] = {
            {0x7,0x5,0x5,0x5,0x7}, // 0
            {0x2,0x6,0x2,0x2,0x7}, // 1
            {0x7,0x1,0x7,0x4,0x7}, // 2
            {0x7,0x1,0x7,0x1,0x7}, // 3
            {0x5,0x5,0x7,0x1,0x1}, // 4
            {0x7,0x4,0x7,0x1,0x7}, // 5
            {0x7,0x4,0x7,0x5,0x7}, // 6
            {0x7,0x1,0x1,0x1,0x1}, // 7
            {0x7,0x5,0x7,0x5,0x7}, // 8
            {0x7,0x5,0x7,0x1,0x7}, // 9
        };
        // Letters A-Z
        static const uint8_t letters[26][5] = {
            {0x2,0x5,0x7,0x5,0x5}, // A
            {0x6,0x5,0x6,0x5,0x6}, // B
            {0x3,0x4,0x4,0x4,0x3}, // C
            {0x6,0x5,0x5,0x5,0x6}, // D
            {0x7,0x4,0x6,0x4,0x7}, // E
            {0x7,0x4,0x6,0x4,0x4}, // F
            {0x3,0x4,0x5,0x5,0x3}, // G
            {0x5,0x5,0x7,0x5,0x5}, // H
            {0x7,0x2,0x2,0x2,0x7}, // I
            {0x1,0x1,0x1,0x5,0x2}, // J
            {0x5,0x5,0x6,0x5,0x5}, // K
            {0x4,0x4,0x4,0x4,0x7}, // L
            {0x5,0x7,0x7,0x5,0x5}, // M
            {0x5,0x7,0x7,0x7,0x5}, // N
            {0x2,0x5,0x5,0x5,0x2}, // O
            {0x7,0x5,0x7,0x4,0x4}, // P
            {0x2,0x5,0x5,0x7,0x3}, // Q
            {0x6,0x5,0x6,0x5,0x5}, // R
            {0x3,0x4,0x2,0x1,0x6}, // S
            {0x7,0x2,0x2,0x2,0x2}, // T
            {0x5,0x5,0x5,0x5,0x7}, // U
            {0x5,0x5,0x5,0x5,0x2}, // V
            {0x5,0x5,0x7,0x7,0x5}, // W
            {0x5,0x5,0x2,0x5,0x5}, // X
            {0x5,0x5,0x2,0x2,0x2}, // Y
            {0x7,0x1,0x2,0x4,0x7}, // Z
        };
        static const uint8_t punct[][5] = {
            {0x0,0x0,0x0,0x0,0x0}, // space
            {0x2,0x2,0x2,0x0,0x2}, // !
            {0x7,0x1,0x2,0x0,0x2}, // ?
            {0x0,0x0,0x7,0x0,0x0}, // -
            {0x0,0x2,0x7,0x2,0x0}, // +
            {0x0,0x7,0x0,0x7,0x0}, // =
            {0x0,0x0,0x0,0x0,0x2}, // .
            {0x0,0x0,0x0,0x2,0x4}, // ,
            {0x0,0x2,0x0,0x2,0x0}, // :
            {0x1,0x2,0x4,0x2,0x1}, // /
        };
        if (ch >= '0' && ch <= '9') return digits[ch - '0'];
        if (ch >= 'A' && ch <= 'Z') return letters[ch - 'A'];
        if (ch >= 'a' && ch <= 'z') return letters[ch - 'a'];
        switch (ch) {
            case ' ': return punct[0];
            case '!': return punct[1];
            case '?': return punct[2];
            case '-': return punct[3];
            case '+': return punct[4];
            case '=': return punct[5];
            case '.': return punct[6];
            case ',': return punct[7];
            case ':': return punct[8];
            case '/': return punct[9];
            default:  return punct[0]; // space fallback
        }
    }

    KL_DECLARE_FIELDS(Canvas2D)
    KL_DECLARE_METHODS(Canvas2D)
    KL_DECLARE_DESCRIBE(Canvas2D)
};

} // namespace koilo
