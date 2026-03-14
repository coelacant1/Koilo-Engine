// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file color_picker.hpp
 * @brief HSV color picker floating panel for the Koilo UI system.
 *
 * Builds a draggable floating panel containing an SV gradient canvas,
 * hue bar, alpha slider, hex input, preview swatch, and OK button.
 * Color changes are previewed live but only applied to the source
 * widget when the user confirms via OK.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/ui/ui_context.hpp>
#include <koilo/systems/ui/render/draw_list.hpp>

namespace koilo {
namespace ui {

/** @class ColorPicker @brief Manages a color picker floating panel. */
class ColorPicker {
public:
    /// Build the picker widgets once. Call during editor setup.
    void Build(UIContext& ctx);

    /// Open the picker for a given ColorField widget.
    void Open(int sourceIdx, float screenX, float screenY);

    /// Close the picker without applying changes.
    void Close();

    /// Apply the current color to the source widget and close.
    void Confirm();

    /// True when the picker panel is visible and active.
    bool IsOpen() const;

    /// Get the pool index of the floating panel widget.
    int PanelIndex() const { return panelIdx_; }

    /// Begin a drag interaction on whichever canvas contains the pointer.
    void HandlePointerDown(float px, float py);

    /// Continue updating the active canvas during a drag.
    void HandlePointerDrag(float px, float py);

    /// End the current drag interaction.
    void HandlePointerUp();

    /// True while a canvas drag is in progress.
    bool IsDragging() const { return activeCanvas_ >= 0; }

    /// Accept hex text input from the hex field (Enter key).
    void ApplyHexInput();

private:
    UIContext* ctx_ = nullptr;
    bool built_ = false;
    bool open_ = false;

    // Widget pool indices.
    int panelIdx_     = -1; ///< Floating panel container.
    int svCanvasIdx_  = -1; ///< SV gradient canvas.
    int hueBarIdx_    = -1; ///< Hue spectrum bar canvas.
    int alphaBarIdx_  = -1; ///< Alpha slider bar canvas.
    int bottomRowIdx_ = -1; ///< Hex + preview row.
    int hexFieldIdx_  = -1; ///< Hex text field.
    int previewIdx_   = -1; ///< Preview swatch.
    int btnRowIdx_    = -1; ///< OK button row.
    int okBtnIdx_     = -1; ///< OK confirmation button.
    int sourceIdx_    = -1; ///< Source ColorField being edited.

    /// Active canvas during drag: -1=none, 0=SV, 1=hue, 2=alpha.
    int activeCanvas_ = -1;

    float hue_ = 0.0f;
    float sat_ = 1.0f;
    float val_ = 1.0f;
    uint8_t alpha_ = 255;
    Color4 currentColor_{255, 255, 255, 255};

    /// Recompute color from HSV and update preview + hex (not source).
    void ApplyColor();

    void UpdatePreview();
    void UpdateHexField();
    void UpdateSource();

    /// Update the given canvas value from pointer coordinates.
    void UpdateCanvasValue(int canvas, float px, float py);

    void PaintSVCanvas(void* rawCtx);
    void PaintHueBar(void* rawCtx);
    void PaintAlphaBar(void* rawCtx);
};

} // namespace ui
} // namespace koilo
