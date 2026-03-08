// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file widget.hpp
 * @brief Lightweight UI widget system - tag-based, no vtable.
 *
 * Tag-based type dispatch enables reflection compatibility while supporting
 * concrete widget types (Label, Panel, Button). All rendering uses direct
 * pixel buffer writes (MCU-friendly, no GPU dependency).
 *
 * @date 01/25/2026
 * @author Coela
 */

#pragma once

#include <string>
#include <vector>
#include <koilo/core/math/vector2d.hpp>
#include <koilo/core/color/color888.hpp>
#include <koilo/assets/font/characters.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @enum WidgetType
 * @brief Tag identifying the concrete widget type.
 */
enum class WidgetType : uint8_t {
    Generic = 0,
    Label,
    Panel,
    Button
};

/**
 * @enum TextAlign
 * @brief Text alignment within a label.
 */
enum class TextAlign : uint8_t {
    Left = 0,
    Center,
    Right
};

/**
 * @class Widget
 * @brief Base UI element - no vtable, reflection-safe.
 *
 * Widgets form a flat parent/children hierarchy. Focus navigation is
 * handled by the UI manager, not by individual widgets.
 */
class Widget {
public:
    // Transform
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    // State
    bool visible = true;
    bool enabled = true;
    bool focusable = false;
    bool focused = false;

    // Identity
    WidgetType type = WidgetType::Generic;
    std::string name;
    std::string onActivate; // script function name to call on activation

    // Label-specific
    std::string text;
    uint8_t textColorR = 255;
    uint8_t textColorG = 255;
    uint8_t textColorB = 255;
    int textScale = 1;
    TextAlign textAlign = TextAlign::Left;

    // Panel/Button-specific
    uint8_t bgColorR = 40;
    uint8_t bgColorG = 40;
    uint8_t bgColorB = 40;
    uint8_t borderColorR = 128;
    uint8_t borderColorG = 128;
    uint8_t borderColorB = 128;
    int borderWidth = 0;

    // Focus highlight color
    uint8_t focusColorR = 0;
    uint8_t focusColorG = 180;
    uint8_t focusColorB = 255;

    // Hierarchy (non-owning)
    Widget* parent = nullptr;
    std::vector<Widget*> children;

    Widget() = default;
    ~Widget() = default;

    // === Hierarchy ===

    void AddChild(Widget* child) {
        if (!child || child == this) return;
        child->parent = this;
        children.push_back(child);
    }

    void RemoveChild(Widget* child) {
        if (!child) return;
        for (auto it = children.begin(); it != children.end(); ++it) {
            if (*it == child) {
                (*it)->parent = nullptr;
                children.erase(it);
                return;
            }
        }
    }

    int GetChildCount() const { return static_cast<int>(children.size()); }

    // === Hit Testing ===

    bool Contains(float px, float py) const {
        float ax = GetAbsoluteX();
        float ay = GetAbsoluteY();
        return px >= ax && px < ax + width && py >= ay && py < ay + height;
    }

    float GetAbsoluteX() const {
        return parent ? parent->GetAbsoluteX() + x : x;
    }

    float GetAbsoluteY() const {
        return parent ? parent->GetAbsoluteY() + y : y;
    }

    // === Property Setters (script-friendly) ===

    void SetPosition(float px, float py) { x = px; y = py; }
    void SetSize(float w, float h) { width = w; height = h; }
    void SetVisible(bool v) { visible = v; }
    void SetEnabled(bool e) { enabled = e; }
    void SetFocusable(bool f) { focusable = f; }
    void SetText(const std::string& t) { text = t; }
    void SetTextColor(int r, int g, int b) {
        textColorR = static_cast<uint8_t>(r);
        textColorG = static_cast<uint8_t>(g);
        textColorB = static_cast<uint8_t>(b);
    }
    void SetTextScale(int s) { textScale = s > 0 ? s : 1; }
    void SetBackgroundColor(int r, int g, int b) {
        bgColorR = static_cast<uint8_t>(r);
        bgColorG = static_cast<uint8_t>(g);
        bgColorB = static_cast<uint8_t>(b);
    }
    void SetBorderColor(int r, int g, int b) {
        borderColorR = static_cast<uint8_t>(r);
        borderColorG = static_cast<uint8_t>(g);
        borderColorB = static_cast<uint8_t>(b);
    }
    void SetTextColor(const Color888& c) { SetTextColor(c.R, c.G, c.B); }
    void SetBackgroundColor(const Color888& c) { SetBackgroundColor(c.R, c.G, c.B); }
    void SetBorderColor(const Color888& c) { SetBorderColor(c.R, c.G, c.B); }
    void SetBorderWidth(int w) { borderWidth = w; }
    void SetOnActivate(const std::string& fn) { onActivate = fn; }
    void SetName(const std::string& n) { name = n; }

    const std::string& GetText() const { return text; }
    const std::string& GetName() const { return name; }
    bool IsVisible() const { return visible; }
    bool IsEnabled() const { return enabled; }
    bool IsFocused() const { return focused; }

    // === Rendering ===

    /**
     * @brief Render this widget into a pixel buffer.
     * @param buffer Pixel buffer (row-major, width * height).
     * @param bufWidth Buffer width in pixels.
     * @param bufHeight Buffer height in pixels.
     */
    void Render(Color888* buffer, int bufWidth, int bufHeight) const {
        if (!visible) return;

        float ax = GetAbsoluteX();
        float ay = GetAbsoluteY();
        int ix = static_cast<int>(ax);
        int iy = static_cast<int>(ay);
        int iw = static_cast<int>(width);
        int ih = static_cast<int>(height);

        switch (type) {
            case WidgetType::Panel:
            case WidgetType::Button:
                RenderPanel(buffer, bufWidth, bufHeight, ix, iy, iw, ih);
                if (focused && focusable)
                    RenderFocusHighlight(buffer, bufWidth, bufHeight, ix, iy, iw, ih);
                if (!text.empty())
                    RenderText(buffer, bufWidth, bufHeight, ix, iy, iw, ih);
                break;
            case WidgetType::Label:
                RenderText(buffer, bufWidth, bufHeight, ix, iy, iw, ih);
                break;
            default:
                break;
        }

        // Render children
        for (const Widget* child : children) {
            child->Render(buffer, bufWidth, bufHeight);
        }
    }

private:
    void RenderPanel(Color888* buffer, int bufW, int bufH,
                     int px, int py, int pw, int ph) const {
        // Fill background
        for (int row = py; row < py + ph; row++) {
            if (row < 0 || row >= bufH) continue;
            for (int col = px; col < px + pw; col++) {
                if (col < 0 || col >= bufW) continue;
                buffer[row * bufW + col] = Color888(bgColorR, bgColorG, bgColorB);
            }
        }
        // Draw border
        if (borderWidth > 0) {
            Color888 bc(borderColorR, borderColorG, borderColorB);
            for (int b = 0; b < borderWidth; b++) {
                // Top + bottom
                for (int col = px; col < px + pw; col++) {
                    if (col < 0 || col >= bufW) continue;
                    if (py + b >= 0 && py + b < bufH)
                        buffer[(py + b) * bufW + col] = bc;
                    if (py + ph - 1 - b >= 0 && py + ph - 1 - b < bufH)
                        buffer[(py + ph - 1 - b) * bufW + col] = bc;
                }
                // Left + right
                for (int row = py; row < py + ph; row++) {
                    if (row < 0 || row >= bufH) continue;
                    if (px + b >= 0 && px + b < bufW)
                        buffer[row * bufW + px + b] = bc;
                    if (px + pw - 1 - b >= 0 && px + pw - 1 - b < bufW)
                        buffer[row * bufW + px + pw - 1 - b] = bc;
                }
            }
        }
    }

    void RenderFocusHighlight(Color888* buffer, int bufW, int bufH,
                              int px, int py, int pw, int ph) const {
        Color888 fc(focusColorR, focusColorG, focusColorB);
        // Draw 1px highlight border outside the widget
        int bx = px - 1, by = py - 1, bw = pw + 2, bh = ph + 2;
        for (int col = bx; col < bx + bw; col++) {
            if (col < 0 || col >= bufW) continue;
            if (by >= 0 && by < bufH) buffer[by * bufW + col] = fc;
            if (by + bh - 1 >= 0 && by + bh - 1 < bufH) buffer[(by + bh - 1) * bufW + col] = fc;
        }
        for (int row = by; row < by + bh; row++) {
            if (row < 0 || row >= bufH) continue;
            if (bx >= 0 && bx < bufW) buffer[row * bufW + bx] = fc;
            if (bx + bw - 1 >= 0 && bx + bw - 1 < bufW) buffer[row * bufW + bx + bw - 1] = fc;
        }
    }

    void RenderText(Color888* buffer, int bufW, int bufH,
                    int px, int py, int pw, int ph) const {
        int glyphW = 8 * textScale;
        int glyphH = 8 * textScale;
        int textW = static_cast<int>(text.length()) * glyphW;

        // Compute text start position based on alignment
        int startX = px;
        int startY = py + (ph - glyphH) / 2; // vertically center

        switch (textAlign) {
            case TextAlign::Center: startX = px + (pw - textW) / 2; break;
            case TextAlign::Right:  startX = px + pw - textW; break;
            default: break;
        }

        Color888 tc(textColorR, textColorG, textColorB);

        for (size_t ci = 0; ci < text.length(); ci++) {
            const uint8_t* glyph = Characters::GetCharacter(text[ci]);
            int charX = startX + static_cast<int>(ci) * glyphW;

            for (int row = 0; row < 8; row++) {
                uint8_t rowBits = glyph[row];
                for (int col = 0; col < 8; col++) {
                    if (rowBits & (0x80 >> col)) {
                        // Scale the pixel
                        for (int sy = 0; sy < textScale; sy++) {
                            for (int sx = 0; sx < textScale; sx++) {
                                int dx = charX + col * textScale + sx;
                                int dy = startY + row * textScale + sy;
                                if (dx >= 0 && dx < bufW && dy >= 0 && dy < bufH) {
                                    buffer[dy * bufW + dx] = tc;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

public:
    KL_BEGIN_FIELDS(Widget)
        KL_FIELD(Widget, x, "X position", 0, 0),
        KL_FIELD(Widget, y, "Y position", 0, 0),
        KL_FIELD(Widget, width, "Width", 0, 0),
        KL_FIELD(Widget, height, "Height", 0, 0),
        KL_FIELD(Widget, visible, "Visible", 0, 1),
        KL_FIELD(Widget, enabled, "Enabled", 0, 1),
        KL_FIELD(Widget, focusable, "Focusable", 0, 1),
        KL_FIELD(Widget, focused, "Focused", 0, 1),
        KL_FIELD(Widget, textScale, "Text scale", 1, 8),
        KL_FIELD(Widget, borderWidth, "Border width", 0, 10)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Widget)
        KL_METHOD_AUTO(Widget, SetPosition, "Set position"),
        KL_METHOD_AUTO(Widget, SetSize, "Set size"),
        KL_METHOD_AUTO(Widget, SetVisible, "Set visible"),
        KL_METHOD_AUTO(Widget, SetEnabled, "Set enabled"),
        KL_METHOD_AUTO(Widget, SetFocusable, "Set focusable"),
        KL_METHOD_AUTO(Widget, SetText, "Set text"),
        KL_METHOD_OVLD(Widget, SetTextColor, void, int, int, int),
        KL_METHOD_OVLD(Widget, SetTextColor, void, const Color888&),
        KL_METHOD_AUTO(Widget, SetTextScale, "Set text scale"),
        KL_METHOD_OVLD(Widget, SetBackgroundColor, void, int, int, int),
        KL_METHOD_OVLD(Widget, SetBackgroundColor, void, const Color888&),
        KL_METHOD_OVLD(Widget, SetBorderColor, void, int, int, int),
        KL_METHOD_OVLD(Widget, SetBorderColor, void, const Color888&),
        KL_METHOD_AUTO(Widget, SetBorderWidth, "Set border width"),
        KL_METHOD_AUTO(Widget, SetOnActivate, "Set activate callback"),
        KL_METHOD_AUTO(Widget, SetName, "Set name"),
        KL_METHOD_AUTO(Widget, GetText, "Get text"),
        KL_METHOD_AUTO(Widget, GetName, "Get name"),
        KL_METHOD_AUTO(Widget, IsVisible, "Is visible"),
        KL_METHOD_AUTO(Widget, IsEnabled, "Is enabled"),
        KL_METHOD_AUTO(Widget, IsFocused, "Is focused"),
        KL_METHOD_AUTO(Widget, AddChild, "Add child widget"),
        KL_METHOD_AUTO(Widget, GetChildCount, "Get child count"),
        KL_METHOD_AUTO(Widget, Contains, "Hit test point")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Widget)
        KL_CTOR0(Widget)
    KL_END_DESCRIBE(Widget)
};

// === Factory functions for concrete widget types ===

/**
 * @brief Create a Label widget (text only, no background).
 */
inline Widget* CreateLabel(const std::string& text, float x, float y) {
    Widget* w = new Widget();
    w->type = WidgetType::Label;
    w->x = x;
    w->y = y;
    w->text = text;
    w->width = static_cast<float>(text.length() * 8);
    w->height = 8.0f;
    return w;
}

/**
 * @brief Create a Panel widget (colored rectangle).
 */
inline Widget* CreatePanel(float x, float y, float w, float h) {
    Widget* widget = new Widget();
    widget->type = WidgetType::Panel;
    widget->x = x;
    widget->y = y;
    widget->width = w;
    widget->height = h;
    return widget;
}

/**
 * @brief Create a Button widget (focusable panel with text).
 */
inline Widget* CreateButton(const std::string& text, float x, float y,
                            float w, float h) {
    Widget* widget = new Widget();
    widget->type = WidgetType::Button;
    widget->x = x;
    widget->y = y;
    widget->width = w;
    widget->height = h;
    widget->text = text;
    widget->focusable = true;
    widget->textAlign = TextAlign::Center;
    return widget;
}

} // namespace koilo
