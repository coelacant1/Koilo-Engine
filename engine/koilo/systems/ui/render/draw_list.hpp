// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file draw_list.hpp
 * @brief Platform-independent UI draw command list.
 *
 * Collects draw commands from the widget tree and provides them
 * to a renderer backend (GL or software). Walks the widget tree
 * recursively, resolving styles and generating rect/text commands.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/ui/widget.hpp>
#include <koilo/systems/ui/style.hpp>
#include <koilo/systems/ui/ui_context.hpp>
#include <koilo/systems/font/font.hpp>
#include <cstdint>
#include <vector>
#include "../../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

// --- Draw command types ---------------------------------------------

enum class DrawCmdType : uint8_t {
    SolidRect,          ///< filled rectangle
    BorderRect,         ///< rectangular outline
    TexturedRect,       ///< textured quad (glyph atlas)
    RoundedRect,        ///< SDF rounded filled rectangle
    RoundedBorderRect,  ///< SDF rounded border rectangle
    PushScissor,        ///< set scissor clip rect
    PopScissor,         ///< restore previous scissor
    Line,               ///< line segment with width
    FilledCircle,       ///< filled circle (SDF)
    CircleOutline,      ///< circle outline
    Triangle            ///< filled triangle
};

/** @struct DrawCmd @brief Single UI draw command. */
struct DrawCmd {
    DrawCmdType type; ///< command type

    float x, y, w, h; ///< rect position/size (pixels, screen space)

    Color4 color; ///< color (RGBA8)

    float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f; ///< texture coordinates (for TexturedRect)
    uint32_t textureHandle = 0; ///< opaque handle (GL texture ID or atlas pointer)

    float borderWidth = 1.0f; ///< border width (for BorderRect / RoundedBorderRect)

    float cornerRadius[4] = {0.0f, 0.0f, 0.0f, 0.0f}; ///< per-corner radii TL, TR, BR, BL

    int scissorX = 0, scissorY = 0, scissorW = 0, scissorH = 0; ///< scissor rect (for PushScissor)

    float x2 = 0.0f, y2 = 0.0f; ///< extra vertex for Triangle (third point)

    KL_BEGIN_FIELDS(DrawCmd)
        KL_FIELD(DrawCmd, type, "Type", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(DrawCmd)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(DrawCmd)
        /* No reflected ctors. */
    KL_END_DESCRIBE(DrawCmd)

};

// --- Draw list ------------------------------------------------------

/** @class UIDrawList @brief Platform-independent list of UI draw commands. */
class UIDrawList {
public:
    /** @brief Clear all commands and reset state. */
    void Clear();

    /** @brief Monotonic counter, bumped each time Clear() is called.
     *
     *  Renderer backends use this as a content-revision token: when it
     *  matches the value seen last frame, the draw list is byte-identical
     *  to the previously-rendered one and a cached vertex stream + draw
     *  sequence may be replayed. Bumping in Clear() is sufficient because
     *  every rebuild path in `UI::PrepareAndRender` calls Clear() before
     *  any Add*, and the early-return / no-rebuild path performs no
     *  mutations on the draw list.
     */
    uint64_t Generation() const { return generation_; }

    /** @brief Return the number of queued commands. */
    size_t Size() const { return commands_.size(); }
    /** @brief Access a command by index. */
    const DrawCmd& operator[](size_t i) const { return commands_[i]; }
    /** @brief Return a pointer to the raw command array. */
    const DrawCmd* Data() const { return commands_.data(); }

    // -- Primitive commands --------------------------------------

    /** @brief Add a filled rectangle. */
    void AddSolidRect(float x, float y, float w, float h, Color4 color);
    /** @brief Add a rectangular border outline. */
    void AddBorderRect(float x, float y, float w, float h,
                       Color4 color, float borderWidth);
    /** @brief Add a textured quad (e.g. glyph atlas). */
    void AddTexturedRect(float x, float y, float w, float h,
                         float u0, float v0, float u1, float v1,
                         Color4 color, uint32_t textureHandle);
    /** @brief Add a rounded filled rectangle (uniform radius). */
    void AddRoundedRect(float x, float y, float w, float h,
                        float radius, Color4 color);
    /** @brief Add a rounded filled rectangle (per-corner radii). */
    void AddRoundedRect(float x, float y, float w, float h,
                        float rTL, float rTR, float rBR, float rBL, Color4 color);
    /** @brief Add a rounded border rectangle (uniform radius). */
    void AddRoundedBorderRect(float x, float y, float w, float h,
                              float radius, Color4 color, float borderWidth);
    /** @brief Add a rounded border rectangle (per-corner radii). */
    void AddRoundedBorderRect(float x, float y, float w, float h,
                              float rTL, float rTR, float rBR, float rBL,
                              Color4 color, float borderWidth);
    /** @brief Add a line segment with width. */
    void AddLine(float x0, float y0, float x1, float y1,
                 float width, Color4 color);
    /** @brief Add a filled circle. */
    void AddFilledCircle(float cx, float cy, float radius, Color4 color);
    /** @brief Add a circle outline. */
    void AddCircleOutline(float cx, float cy, float radius,
                          float lineWidth, Color4 color);
    /** @brief Add a filled triangle. */
    void AddTriangle(float x0, float y0, float x1, float y1,
                     float x2, float y2, Color4 color);
    /** @brief Push an integer scissor clip rect. */
    void PushScissor(int x, int y, int w, int h);
    /** @brief Push a float scissor clip rect (floor/ceil converted). */
    void PushScissor(float x, float y, float w, float h);
    /** @brief Restore the previous scissor rect. */
    void PopScissor();

    friend class CanvasDrawContext;

    /// Emit text glyphs into the draw list (for debug overlays).
    /// Requires a font to be set via BuildFromContext or SetFont.
    void DrawText(const char* text, float x, float y,
                  float fontSize, Color4 color);

    // -- Widget tree -> draw commands -----------------------------

    /** @brief Build draw commands for the entire UI tree. */
    void BuildFromContext(UIContext& ctx, font::Font* font,
                          uint32_t fontAtlasTexture,
                          font::Font* boldFont = nullptr,
                          uint32_t boldFontAtlasTexture = 0);

private:
    std::vector<DrawCmd> commands_; ///< queued draw commands
    uint64_t generation_ = 0;       ///< content revision; bumped on Clear()

    /** @struct ScissorRect @brief Saved scissor clip rectangle. */
    struct ScissorRect {
        int x, y, w, h;

        KL_BEGIN_FIELDS(ScissorRect)
            KL_FIELD(ScissorRect, x, "X", 0, 0),
            KL_FIELD(ScissorRect, y, "Y", 0, 0),
            KL_FIELD(ScissorRect, w, "W", 0, 0),
            KL_FIELD(ScissorRect, h, "H", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(ScissorRect)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(ScissorRect)
            /* No reflected ctors. */
        KL_END_DESCRIBE(ScissorRect)
    };
    std::vector<ScissorRect> scissorStack_; ///< nested scissor state stack

    font::Font* font_ = nullptr; ///< font used for text rendering
    font::Font* boldFont_ = nullptr; ///< bold font for weight >= 700
    uint32_t fontAtlasTexture_ = 0; ///< GL texture handle for the font atlas
    uint32_t boldFontAtlasTexture_ = 0; ///< GL texture handle for bold font atlas

    void EmitWidget(const Widget& widget, int widgetIdx, const WidgetPool& pool,
                    const StringTable& strings, const Theme& theme);
    void EmitText(const Widget& widget, const Rect& r, const Style& style,
                  const StringTable& strings);
    void EmitMenuItem(const Widget& widget, const Rect& r, const Style& style,
                      const StringTable& strings);
    void EmitTextCentered(const Widget& widget, const Rect& r,
                          const Style& style, const StringTable& strings);
    void EmitTextField(const Widget& widget, const Rect& r,
                       const Style& style, const StringTable& strings);
    void EmitCheckbox(const Widget& widget, const Rect& r,
                      const Style& style);
    void EmitSlider(const Widget& widget, const Rect& r,
                    const Style& style);
    void EmitSeparator(const Widget& widget, const Rect& r,
                       const Style& style);
    void EmitTreeNode(const Widget& widget, const Rect& r,
                      const Style& style, const StringTable& strings,
                      const WidgetPool& pool, int widgetIdx);
    void EmitScrollbar(const Widget& widget, const Rect& r,
                       const Style& style);
    void EmitDropdown(const Widget& widget, const Rect& r,
                      const Style& style, const StringTable& strings,
                      const WidgetPool& pool, int widgetIdx);
    void EmitDropdownPopup(const Widget& widget, const Rect& r,
                           const Style& style, const StringTable& strings,
                           const WidgetPool& pool);
    void EmitTabBar(const Widget& widget, const Rect& r,
                    const Style& style);
    void EmitImage(const Widget& widget, const Rect& r);
    void EmitColorField(const Widget& widget, const Rect& r,
                        const Style& style);
    void EmitProgressBar(const Widget& widget, const Rect& r,
                         const Style& style);
    void EmitToggleSwitch(const Widget& widget, const Rect& r,
                          const Style& style);
    void EmitRadioButton(const Widget& widget, const Rect& r,
                         const Style& style);
    void EmitNumberSpinner(const Widget& widget, const Rect& r,
                           const Style& style, const StringTable& strings);
    void EmitFloatingPanel(const Widget& widget, const Rect& r,
                           const Style& style, const StringTable& strings);
    void EmitMinimizedPanelTray(const WidgetPool& pool,
                                const StringTable& strings,
                                float viewportW, float viewportH);
    void EmitDividers(const Widget& parent, const WidgetPool& pool);
    void EmitTextGlyphs(const char* text, float x, float y,
                        float fontSize, Color4 color, uint16_t fontWeight = 400);
    void EmitTooltip(const char* text, float mouseX, float mouseY,
                     float vpW, float vpH);
    void EmitDragOverlay(const UIContext& ctx, const StringTable& strings);
    static uint32_t DecodeUTF8(const char* text, size_t len, size_t& i);

    /// Deferred dropdown popup entry (rendered as overlay after main tree).
    struct DeferredDropdown {
        const Widget* widget;
        Rect rect;
        Style style;
        const WidgetPool* pool;
        const StringTable* strings;
        int widgetIdx;

        KL_BEGIN_FIELDS(DeferredDropdown)
            KL_FIELD(DeferredDropdown, widget, "Widget", 0, 0),
            KL_FIELD(DeferredDropdown, rect, "Rect", 0, 0),
            KL_FIELD(DeferredDropdown, style, "Style", 0, 0),
            KL_FIELD(DeferredDropdown, pool, "Pool", 0, 0),
            KL_FIELD(DeferredDropdown, strings, "Strings", 0, 0),
            KL_FIELD(DeferredDropdown, widgetIdx, "Widget idx", -2147483648, 2147483647)
        KL_END_FIELDS

        KL_BEGIN_METHODS(DeferredDropdown)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(DeferredDropdown)
            /* No reflected ctors. */
        KL_END_DESCRIBE(DeferredDropdown)

    };
    std::vector<DeferredDropdown> deferredDropdowns_; ///< Open dropdowns to render on top.

    /// Deferred floating panel entry. Floating panels logically escape
    /// their parent's clipping; queueing here lets them render after all
    /// parent scissors have been popped. The whole widget subtree is
    /// re-emitted via EmitWidget at drain time so chrome draws first
    /// and children render on top -- preserving in-tree z-order.
    struct DeferredFloatingPanel {
        const Widget* widget;
        int widgetIdx;
        const WidgetPool* pool;
        const StringTable* strings;
        const Theme* theme;
    };
    std::vector<DeferredFloatingPanel> deferredFloatingPanels_;
    bool drainingFloatingPanels_ = false;

    KL_BEGIN_FIELDS(UIDrawList)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(UIDrawList)
        KL_METHOD_AUTO(UIDrawList, Clear, "Clear"),
        KL_METHOD_AUTO(UIDrawList, Size, "Size"),
        KL_METHOD_AUTO(UIDrawList, Data, "Data"),
        KL_METHOD_AUTO(UIDrawList, AddSolidRect, "Add solid rect"),
        KL_METHOD_AUTO(UIDrawList, AddBorderRect, "Add border rect"),
        KL_METHOD_AUTO(UIDrawList, AddTexturedRect, "Add textured rect"),
        KL_METHOD_AUTO(UIDrawList, PopScissor, "Pop scissor")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(UIDrawList)
        /* No reflected ctors. */
    KL_END_DESCRIBE(UIDrawList)

};

/**
 * @class CanvasDrawContext
 * @brief Drawing context passed to Canvas2D onPaint callbacks.
 *
 * Coordinates are relative to the canvas widget's top-left corner.
 */
class CanvasDrawContext {
public:
    /** @brief Construct a canvas drawing context. */
    CanvasDrawContext(UIDrawList& dl, float originX, float originY,
                     float width, float height, font::Font* font)
        : dl_(dl), ox_(originX), oy_(originY), w_(width), h_(height), font_(font) {}

    /** @brief Return the canvas width. */
    float Width() const { return w_; }
    /** @brief Return the canvas height. */
    float Height() const { return h_; }

    /** @brief Draw a filled rectangle. */
    void DrawRect(float x, float y, float w, float h, Color4 color) {
        dl_.AddSolidRect(ox_ + x, oy_ + y, w, h, color);
    }
    /** @brief Draw a rounded filled rectangle. */
    void DrawRoundedRect(float x, float y, float w, float h, float r, Color4 color) {
        dl_.AddRoundedRect(ox_ + x, oy_ + y, w, h, r, color);
    }
    /** @brief Draw a border rectangle. */
    void DrawBorderRect(float x, float y, float w, float h, Color4 color, float bw = 1.0f) {
        dl_.AddBorderRect(ox_ + x, oy_ + y, w, h, color, bw);
    }
    /** @brief Draw a line segment. */
    void DrawLine(float x0, float y0, float x1, float y1, float width, Color4 color) {
        dl_.AddLine(ox_ + x0, oy_ + y0, ox_ + x1, oy_ + y1, width, color);
    }
    /** @brief Draw a filled circle. */
    void DrawCircle(float cx, float cy, float r, Color4 color) {
        dl_.AddFilledCircle(ox_ + cx, oy_ + cy, r, color);
    }
    /** @brief Draw a circle outline. */
    void DrawCircleOutline(float cx, float cy, float r, float lineW, Color4 color) {
        dl_.AddCircleOutline(ox_ + cx, oy_ + cy, r, lineW, color);
    }
    /** @brief Draw a filled triangle. */
    void DrawTriangle(float x0, float y0, float x1, float y1,
                      float x2, float y2, Color4 color) {
        dl_.AddTriangle(ox_ + x0, oy_ + y0, ox_ + x1, oy_ + y1, ox_ + x2, oy_ + y2, color);
    }
    /** @brief Draw text at the given position. */
    void DrawText(float x, float y, const char* text, Color4 color, float fontSize = 14.0f);

private:
    UIDrawList& dl_; ///< underlying draw list
    float ox_, oy_, w_, h_; ///< canvas origin and dimensions
    font::Font* font_; ///< font for text rendering

    KL_BEGIN_FIELDS(CanvasDrawContext)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(CanvasDrawContext)
        KL_METHOD_AUTO(CanvasDrawContext, Height, "Height"),
        KL_METHOD_AUTO(CanvasDrawContext, DrawRect, "Draw rect"),
        KL_METHOD_AUTO(CanvasDrawContext, DrawRoundedRect, "Draw rounded rect"),
        KL_METHOD_AUTO(CanvasDrawContext, DrawBorderRect, "Draw border rect"),
        KL_METHOD_AUTO(CanvasDrawContext, DrawLine, "Draw line"),
        KL_METHOD_AUTO(CanvasDrawContext, DrawCircle, "Draw circle"),
        KL_METHOD_AUTO(CanvasDrawContext, DrawCircleOutline, "Draw circle outline"),
        KL_METHOD_AUTO(CanvasDrawContext, DrawTriangle, "Draw triangle"),
        KL_METHOD_AUTO(CanvasDrawContext, DrawText, "Draw text")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CanvasDrawContext)
        /* No reflected ctors. */
    KL_END_DESCRIBE(CanvasDrawContext)

};

} // namespace ui
} // namespace koilo
