// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_renderer.hpp
 * @brief Abstract interface for UI renderers (GPU and software).
 *
 * Provides a common interface consumed by the UI class to render
 * a UIDrawList regardless of whether the backend is a GPU RHI
 * pipeline or a CPU software rasterizer.
 *
 * @date 03/29/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/ui/render/draw_list.hpp>
#include <koilo/systems/font/font.hpp>
#include <cstdint>

namespace koilo {
namespace ui {

/// Abstract UI renderer interface.
///
/// UIRHIRenderer implements this for all backends (GPU and software RHI)
/// so the UI class can drive rendering through a single code path.
class IUIRenderer {
public:
    virtual ~IUIRenderer() = default;

    /// Release renderer resources.
    virtual void Shutdown() = 0;

    /// Check if the renderer has been initialized.
    virtual bool IsInitialized() const = 0;

    /// Set the regular font and manage its atlas.
    /// GPU renderers upload the atlas to a texture; software renderers
    /// store a pointer.  Returns an opaque atlas handle (0 if SW).
    virtual uint32_t SetFont(font::Font* font) = 0;

    /// Set the bold font and manage its atlas.
    virtual uint32_t SetBoldFont(font::Font* font) = 0;

    /// Re-upload dirty font atlases after glyph rasterization.
    /// Called after BuildFromContext may have added new glyphs.
    /// Returns updated handles via the out parameters.
    virtual void SyncFontAtlases(font::Font* font, uint32_t& fontHandle,
                                 font::Font* boldFont, uint32_t& boldHandle) = 0;

    /// Render a draw list to the active output (screen or pixel buffer).
    /// @param drawList  The draw commands to render.
    /// @param viewportW  Viewport width in pixels.
    /// @param viewportH  Viewport height in pixels.
    virtual void Render(const UIDrawList& drawList,
                        int viewportW, int viewportH) = 0;

    /// Access the rendered pixel buffer (software renderer only).
    /// GPU renderers return nullptr.
    virtual const uint8_t* Pixels() const { return nullptr; }

    /// Whether this is a software (CPU) renderer.
    virtual bool IsSoftware() const = 0;
};

} // namespace ui
} // namespace koilo
