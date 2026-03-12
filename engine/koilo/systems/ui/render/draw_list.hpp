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
    SolidRect,       // filled rectangle
    BorderRect,      // rectangular outline
    TexturedRect,    // textured quad (glyph atlas)
    RoundedRect,     // SDF rounded filled rectangle
    RoundedBorderRect, // SDF rounded border rectangle
    PushScissor,     // set scissor clip rect
    PopScissor       // restore previous scissor
};

struct DrawCmd {
    DrawCmdType type;

    // Rect position/size (in pixels, screen space)
    float x, y, w, h;

    // Color (RGBA8)
    Color4 color;

    // Texture info (for TexturedRect)
    float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
    uint32_t textureHandle = 0;  // opaque handle (GL texture ID or atlas pointer)

    // Border width (for BorderRect / RoundedBorderRect)
    float borderWidth = 1.0f;

    // Corner radii (for RoundedRect / RoundedBorderRect) - TL, TR, BR, BL
    float cornerRadius[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    // Scissor rect (for PushScissor)
    int scissorX = 0, scissorY = 0, scissorW = 0, scissorH = 0;

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

class UIDrawList {
public:
    void Clear();

    size_t Size() const { return commands_.size(); }
    const DrawCmd& operator[](size_t i) const { return commands_[i]; }
    const DrawCmd* Data() const { return commands_.data(); }

    // -- Primitive commands --------------------------------------

    void AddSolidRect(float x, float y, float w, float h, Color4 color);
    void AddBorderRect(float x, float y, float w, float h,
                       Color4 color, float borderWidth);
    void AddTexturedRect(float x, float y, float w, float h,
                         float u0, float v0, float u1, float v1,
                         Color4 color, uint32_t textureHandle);
    void AddRoundedRect(float x, float y, float w, float h,
                        float radius, Color4 color);
    void AddRoundedRect(float x, float y, float w, float h,
                        float rTL, float rTR, float rBR, float rBL, Color4 color);
    void AddRoundedBorderRect(float x, float y, float w, float h,
                              float radius, Color4 color, float borderWidth);
    void AddRoundedBorderRect(float x, float y, float w, float h,
                              float rTL, float rTR, float rBR, float rBL,
                              Color4 color, float borderWidth);
    void PushScissor(int x, int y, int w, int h);
    void PushScissor(float x, float y, float w, float h);
    void PopScissor();

    // -- Widget tree -> draw commands -----------------------------

    /// Build draw commands for the entire UI tree.
    void BuildFromContext(UIContext& ctx, font::Font* font,
                          uint32_t fontAtlasTexture);

private:
    std::vector<DrawCmd> commands_;

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
    std::vector<ScissorRect> scissorStack_;

    font::Font* font_ = nullptr;
    uint32_t fontAtlasTexture_ = 0;

    void EmitWidget(const Widget& widget, int widgetIdx, const WidgetPool& pool,
                    const StringTable& strings, const Theme& theme);
    void EmitText(const Widget& widget, const Rect& r, const Style& style,
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
                      const Style& style, const StringTable& strings);
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
                        float fontSize, Color4 color);
    void EmitTooltip(const char* text, float mouseX, float mouseY,
                     float vpW, float vpH);
    static uint32_t DecodeUTF8(const char* text, size_t len, size_t& i);

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

} // namespace ui
} // namespace koilo
