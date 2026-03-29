// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_sw_renderer.hpp
 * @brief CPU software UI renderer fallback.
 *
 * Renders a UIDrawList to an RGBA8888 pixel buffer without any
 * GPU dependency.  Used for headless rendering, testing, or
 * platforms without OpenGL support.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/ui/render/draw_list.hpp>
#include <koilo/systems/ui/render/ui_renderer.hpp>
#include <koilo/systems/font/font.hpp>
#include <cstdint>
#include <vector>
#include "../../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

// --- Software UI Renderer -------------------------------------------

/** @class UISWRenderer @brief CPU software UI renderer fallback. */
class UISWRenderer : public IUIRenderer {
public:
    UISWRenderer() = default;
    ~UISWRenderer() override = default;

    // -- IUIRenderer interface ------------------------------------------

    void Shutdown() override;
    bool IsInitialized() const override { return true; }
    bool IsSoftware() const override { return true; }

    uint32_t SetFont(font::Font* font) override;
    uint32_t SetBoldFont(font::Font* font) override;
    void SyncFontAtlases(font::Font* font, uint32_t& fontHandle,
                         font::Font* boldFont, uint32_t& boldHandle) override;

    /// Render via IUIRenderer interface.  Resizes and clears internally.
    void Render(const UIDrawList& drawList,
                int viewportW, int viewportH) override;

    const uint8_t* Pixels() const override { return pixels_.data(); }

    // -- SW-specific methods --------------------------------------------

    /// Resize the pixel buffer. Must be called before RenderDirect().
    void Resize(int width, int height);

    /// Clear the pixel buffer to transparent.
    void Clear();

    /// Clear with a specific color.
    void Clear(Color4 color);

    /// Direct render with explicit atlas pointers (low-level).
    void RenderDirect(const UIDrawList& drawList,
                      const font::GlyphAtlas* atlas = nullptr,
                      const font::GlyphAtlas* boldAtlas = nullptr);

    /** @brief Return the buffer width in pixels. */
    int Width()  const { return width_; }
    /** @brief Return the buffer height in pixels. */
    int Height() const { return height_; }
    /** @brief Return a mutable pointer to the RGBA pixel data. */
    uint8_t* MutablePixels() { return pixels_.data(); }

private:
    int width_  = 0; ///< buffer width in pixels
    int height_ = 0; ///< buffer height in pixels
    std::vector<uint8_t> pixels_; ///< RGBA8888 pixel buffer

    font::Font* font_     = nullptr; ///< regular font (stored by SetFont)
    font::Font* boldFont_ = nullptr; ///< bold font (stored by SetBoldFont)

    int scissorX_ = 0, scissorY_ = 0; ///< current scissor origin
    int scissorW_ = 0, scissorH_ = 0; ///< current scissor size

    /** @struct ScissorState @brief Saved scissor clip state. */
    struct ScissorState {
        int x, y, w, h;

        KL_BEGIN_FIELDS(ScissorState)
            KL_FIELD(ScissorState, x, "X", 0, 0),
            KL_FIELD(ScissorState, y, "Y", 0, 0),
            KL_FIELD(ScissorState, w, "W", 0, 0),
            KL_FIELD(ScissorState, h, "H", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(ScissorState)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(ScissorState)
            /* No reflected ctors. */
        KL_END_DESCRIBE(ScissorState)
    };
    std::vector<ScissorState> scissorStack_; ///< nested scissor state stack

    void BlendPixel(int x, int y, Color4 c);
    void FillRect(int x, int y, int w, int h, Color4 color);
    void DrawBorder(int x, int y, int w, int h, int bw, Color4 color);
    void BlitAtlasRect(const DrawCmd& cmd, const font::GlyphAtlas& atlas);
    static float RoundedRectCoverage(int px, int py, int rx, int ry,
                                     int rw, int rh,
                                     float rTL, float rTR,
                                     float rBR, float rBL);
    void FillRoundedRect(int x, int y, int w, int h,
                         const float radii[4], Color4 color);
    void DrawRoundedBorder(int x, int y, int w, int h,
                           const float radii[4], int bw, Color4 color);
    void DrawLine(float x0, float y0, float x1, float y1,
                  float width, Color4 color);
    void DrawFilledCircle(float cx, float cy, float radius, Color4 color);
    void DrawCircleOutline(float cx, float cy, float radius,
                           float lineWidth, Color4 color);
    void DrawTriangle(float x0, float y0, float x1, float y1,
                      float x2, float y2, Color4 color);

    KL_BEGIN_FIELDS(UISWRenderer)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(UISWRenderer)
        KL_METHOD_AUTO(UISWRenderer, Width, "Width"),
        KL_METHOD_AUTO(UISWRenderer, Height, "Height")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(UISWRenderer)
        KL_CTOR0(UISWRenderer)
    KL_END_DESCRIBE(UISWRenderer)

};

} // namespace ui
} // namespace koilo
