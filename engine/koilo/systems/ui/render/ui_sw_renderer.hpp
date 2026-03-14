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
#include <koilo/systems/font/font.hpp>
#include <cstdint>
#include <vector>
#include "../../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

// --- Software UI Renderer -------------------------------------------

/** @class UISWRenderer @brief CPU software UI renderer fallback. */
class UISWRenderer {
public:
    UISWRenderer() = default;

    /// Resize the pixel buffer. Must be called before Render().
    void Resize(int width, int height);

    /// Clear the pixel buffer to transparent.
    void Clear();

    /// Clear with a specific color.
    void Clear(Color4 color);

    /// Render a draw list to the pixel buffer.
    void Render(const UIDrawList& drawList,
                const font::GlyphAtlas* atlas = nullptr);

    /** @brief Return the buffer width in pixels. */
    int Width()  const { return width_; }
    /** @brief Return the buffer height in pixels. */
    int Height() const { return height_; }
    /** @brief Return a read-only pointer to the RGBA pixel data. */
    const uint8_t* Pixels() const { return pixels_.data(); }
    /** @brief Return a mutable pointer to the RGBA pixel data. */
    uint8_t* Pixels() { return pixels_.data(); }

private:
    int width_  = 0; ///< buffer width in pixels
    int height_ = 0; ///< buffer height in pixels
    std::vector<uint8_t> pixels_; ///< RGBA8888 pixel buffer

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
    static bool IsInsideRoundedRect(int px, int py, int rx, int ry,
                                    int rw, int rh, float radius);
    void FillRoundedRect(int x, int y, int w, int h,
                         float radius, Color4 color);
    void DrawRoundedBorder(int x, int y, int w, int h,
                           float radius, int bw, Color4 color);

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
