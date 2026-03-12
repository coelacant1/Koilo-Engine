// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file style.hpp
 * @brief Style properties, pseudo-states, and theme system for the UI engine.
 *
 * Styles are plain structs indexed by widget tag and interaction state.
 * A Theme is a flat table of styles that can be swapped at runtime.
 *
 * @date 03/08/2026
 * @author Coela
 */

#pragma once

#include "widget.hpp"
#include "../../registry/reflect_macros.hpp"
#include <unordered_map>
#include <array>

namespace koilo {
namespace ui {

// --- Pseudo-States ---

enum class PseudoState : uint8_t {
    Normal = 0,
    Hovered,
    Pressed,
    Focused,
    FocusVisible,  // keyboard-only focus (Tab navigation)
    Selected,
    Disabled,
    Count
};

// --- Style ---

struct BorderStyle {
    Color4 color{80, 80, 80, 255};
    float width = 0.0f;
    float radiusTL = 0.0f; // top-left
    float radiusTR = 0.0f; // top-right
    float radiusBR = 0.0f; // bottom-right
    float radiusBL = 0.0f; // bottom-left

    /// Set all corners to the same radius.
    void SetRadius(float r) { radiusTL = radiusTR = radiusBR = radiusBL = r; }
    /// Set per-corner radii (CSS order: TL TR BR BL).
    void SetRadius(float tl, float tr, float br, float bl) {
        radiusTL = tl; radiusTR = tr; radiusBR = br; radiusBL = bl;
    }
    /// True if any corner has a non-zero radius.
    bool HasRadius() const { return radiusTL > 0 || radiusTR > 0 || radiusBR > 0 || radiusBL > 0; }
    /// True if all corners share the same radius.
    bool IsUniform() const { return radiusTL == radiusTR && radiusTR == radiusBR && radiusBR == radiusBL; }
    /// Get uniform radius (returns TL; valid when IsUniform()).
    float Uniform() const { return radiusTL; }

    KL_BEGIN_FIELDS(BorderStyle)
        KL_FIELD(BorderStyle, width, "Width", 0, 0),
        KL_FIELD(BorderStyle, radiusTL, "RadiusTL", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(BorderStyle)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(BorderStyle)
        /* No reflected ctors. */
    KL_END_DESCRIBE(BorderStyle)

};

struct Shadow {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float blur = 0.0f;
    float spread = 0.0f;
    Color4 color{0, 0, 0, 128};
    bool active = false;
};

struct TextShadow {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float blur = 0.0f;
    Color4 color{0, 0, 0, 128};
    bool active = false;
};

struct Gradient {
    Color4 from{0, 0, 0, 0};
    Color4 to{0, 0, 0, 0};
    float angle = 180.0f; // degrees, 180 = top-to-bottom
    bool active = false;
};

struct Transition {
    float duration = 0.0f;   // seconds
    bool active = false;
};

struct Transform2D {
    float rotate = 0.0f;       // degrees
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float translateX = 0.0f;
    float translateY = 0.0f;
    float originX = 0.5f;     // 0.0 = left, 0.5 = center, 1.0 = right
    float originY = 0.5f;     // 0.0 = top, 0.5 = center, 1.0 = bottom
    bool active = false;

    bool IsIdentity() const {
        return rotate == 0.0f && scaleX == 1.0f && scaleY == 1.0f &&
               translateX == 0.0f && translateY == 0.0f;
    }
};

struct Style {
    Color4 background{40, 40, 40, 255};
    Color4 textColor{220, 220, 220, 255};
    Color4 caretColor{220, 220, 220, 255};       // text cursor color
    Color4 accentColor{80, 140, 200, 255};        // checkbox/slider accent
    Color4 placeholderColor{120, 120, 140, 180};  // placeholder text color
    BorderStyle border{};
    float fontSize = 14.0f;
    float opacity = 1.0f;
    Edges padding{0.0f, 0.0f, 0.0f, 0.0f};
    Shadow shadow{};
    TextShadow textShadow{};
    Gradient gradient{};
    Transition transition{};
    Transform2D transform{};
    // Flag: has this style been explicitly set, or is it inherited?
    bool isSet = false;

    KL_BEGIN_FIELDS(Style)
        KL_FIELD(Style, fontSize, "Font size", 0, 0),
        KL_FIELD(Style, opacity, "Opacity", 0, 1),
        KL_FIELD(Style, isSet, "Is set", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Style)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Style)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Style)

};

// --- Theme ---

/// Theme holds one Style per (WidgetTag, PseudoState) combination.
/// Lookup: theme.styles[tag][state]
class Theme {
public:
    static constexpr int TAG_COUNT   = static_cast<int>(WidgetTag::Count);
    static constexpr int STATE_COUNT = static_cast<int>(PseudoState::Count);

    Theme() { SetDefaults(); }

    void Set(WidgetTag tag, PseudoState state, const Style& style) {
        auto& s = styles_[static_cast<int>(tag)][static_cast<int>(state)];
        s = style;
        s.isSet = true;
    }

    const Style& Get(WidgetTag tag, PseudoState state) const {
        const auto& s = styles_[static_cast<int>(tag)][static_cast<int>(state)];
        if (s.isSet) return s;
        // Fall back to Normal state
        return styles_[static_cast<int>(tag)][static_cast<int>(PseudoState::Normal)];
    }

    /// Set a per-widget style override (KSS-computed, not global theme).
    void SetWidget(int widgetIdx, PseudoState state, const Style& style) {
        auto& arr = widgetStyles_[widgetIdx];
        auto& s = arr[static_cast<int>(state)];
        s = style;
        s.isSet = true;
    }

    /// Get per-widget style override, returns nullptr if none set.
    const Style* GetWidget(int widgetIdx, PseudoState state) const {
        auto it = widgetStyles_.find(widgetIdx);
        if (it == widgetStyles_.end()) return nullptr;
        const auto& s = it->second[static_cast<int>(state)];
        if (s.isSet) return &s;
        // Fall back to Normal state for this widget
        const auto& normal = it->second[static_cast<int>(PseudoState::Normal)];
        if (normal.isSet) return &normal;
        return nullptr;
    }

    /// Resolve the effective style for a widget based on its current state.
    /// Checks per-widget overrides first, then falls back to tag-based theme.
    Style Resolve(const Widget& w, int widgetIdx = -1) const {
        PseudoState state = PseudoState::Normal;
        if (!w.flags.enabled)              state = PseudoState::Disabled;
        else if (w.flags.pressed)          state = PseudoState::Pressed;
        else if (w.flags.focusVisible)     state = PseudoState::FocusVisible;
        else if (w.flags.hovered)          state = PseudoState::Hovered;
        else if (w.flags.selected)         state = PseudoState::Selected;
        else if (w.flags.focused)          state = PseudoState::Focused;
        if (widgetIdx >= 0) {
            const Style* ws = GetWidget(widgetIdx, state);
            if (ws) return *ws;
            if (state == PseudoState::FocusVisible) {
                ws = GetWidget(widgetIdx, PseudoState::Focused);
                if (ws) return *ws;
            }
        }
        const Style& s = Get(w.tag, state);
        if (state == PseudoState::FocusVisible && !s.isSet)
            return Get(w.tag, PseudoState::Focused);
        return s;
    }

    /// Clear all per-widget styles (called before re-applying stylesheet).
    void ClearWidgetStyles() { widgetStyles_.clear(); }

    /// Transition animation state for a widget
    struct TransitionState {
        Style current;      // currently rendered (interpolated) style
        Style target;       // target style (from current pseudo-state)
        float remaining;    // seconds remaining in transition
        float duration;     // total transition duration
        bool active = false;
    };

    /// Update transitions for all widgets. Call once per frame with delta time.
    void UpdateTransitions(float dt) {
        for (auto it = transitions_.begin(); it != transitions_.end(); ) {
            auto& ts = it->second;
            if (!ts.active) { it = transitions_.erase(it); continue; }
            ts.remaining -= dt;
            if (ts.remaining <= 0.0f) {
                ts.current = ts.target;
                ts.active = false;
                it = transitions_.erase(it);
            } else {
                float t = 1.0f - (ts.remaining / ts.duration);
                // Ease-out cubic for smooth feel
                float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
                ts.current = LerpStyle(ts.current, ts.target, ease);
                ++it;
            }
        }
    }

    /// Resolve with transition: returns interpolated style if transitioning.
    Style ResolveAnimated(const Widget& w, int widgetIdx = -1) const {
        Style target = Resolve(w, widgetIdx);
        if (widgetIdx < 0) return target;

        float duration = target.transition.active ? target.transition.duration : 0.0f;
        if (duration <= 0.0f) {
            // No transition - remove any existing and return target directly
            transitions_.erase(widgetIdx);
            return target;
        }

        auto it = transitions_.find(widgetIdx);
        if (it != transitions_.end()) {
            auto& ts = it->second;
            // Check if target changed
            if (!StyleEqual(ts.target, target)) {
                ts.current = ts.current; // keep current interpolated position
                ts.target = target;
                ts.remaining = duration;
                ts.duration = duration;
                ts.active = true;
            }
            return ts.current;
        } else {
            // No active transition - just return target, but store for future
            TransitionState ts;
            ts.current = target;
            ts.target = target;
            ts.remaining = 0.0f;
            ts.duration = duration;
            ts.active = false;
            transitions_[widgetIdx] = ts;
            return target;
        }
    }

    /// Notify that a widget's state changed (triggers transition if configured).
    void NotifyStateChange(const Widget& w, int widgetIdx) {
        Style target = Resolve(w, widgetIdx);
        float duration = target.transition.active ? target.transition.duration : 0.0f;
        if (duration <= 0.0f) return;

        auto it = transitions_.find(widgetIdx);
        if (it != transitions_.end()) {
            auto& ts = it->second;
            if (!StyleEqual(ts.target, target)) {
                // Start transition from current interpolated style
                ts.target = target;
                ts.remaining = duration;
                ts.duration = duration;
                ts.active = true;
            }
        } else {
            // First time - start from current resolve (no visual jump)
            TransitionState ts;
            ts.current = target;
            ts.target = target;
            ts.remaining = 0.0f;
            ts.duration = duration;
            ts.active = false;
            transitions_[widgetIdx] = ts;
        }
    }

private:
    void SetDefaults() {
        // Panel
        Style panel;
        panel.background = {30, 30, 30, 255};
        panel.isSet = true;
        styles_[static_cast<int>(WidgetTag::Panel)][static_cast<int>(PseudoState::Normal)] = panel;

        // Label
        Style label;
        label.background = {0, 0, 0, 0};
        label.textColor = {210, 210, 210, 255};
        label.padding = {2.0f, 2.0f, 2.0f, 2.0f};
        label.isSet = true;
        styles_[static_cast<int>(WidgetTag::Label)][static_cast<int>(PseudoState::Normal)] = label;

        // Button
        Style btn;
        btn.background = {60, 60, 60, 255};
        btn.textColor = {230, 230, 230, 255};
        btn.border = {{90, 90, 90, 255}, 1.0f, 4.0f};
        btn.padding = {2.0f, 4.0f, 2.0f, 4.0f};
        btn.isSet = true;
        styles_[static_cast<int>(WidgetTag::Button)][static_cast<int>(PseudoState::Normal)] = btn;

        Style btnHover;
        btnHover.background = {75, 75, 75, 255};
        btnHover.textColor = {255, 255, 255, 255};
        btnHover.border = {{120, 120, 120, 255}, 1.0f, 4.0f};
        btnHover.padding = {2.0f, 4.0f, 2.0f, 4.0f};
        btnHover.isSet = true;
        styles_[static_cast<int>(WidgetTag::Button)][static_cast<int>(PseudoState::Hovered)] = btnHover;

        Style btnPressed;
        btnPressed.background = {45, 45, 45, 255};
        btnPressed.textColor = {200, 200, 200, 255};
        btnPressed.border = {{100, 100, 100, 255}, 1.0f, 4.0f};
        btnPressed.padding = {2.0f, 4.0f, 2.0f, 4.0f};
        btnPressed.isSet = true;
        styles_[static_cast<int>(WidgetTag::Button)][static_cast<int>(PseudoState::Pressed)] = btnPressed;

        // TextField
        Style tf;
        tf.background = {25, 25, 25, 255};
        tf.textColor = {220, 220, 220, 255};
        tf.border = {{70, 70, 70, 255}, 1.0f, 2.0f};
        tf.padding = {2.0f, 4.0f, 2.0f, 4.0f};
        tf.isSet = true;
        styles_[static_cast<int>(WidgetTag::TextField)][static_cast<int>(PseudoState::Normal)] = tf;

        Style tfFocused;
        tfFocused.background = {25, 25, 25, 255};
        tfFocused.textColor = {255, 255, 255, 255};
        tfFocused.border = {{80, 140, 200, 255}, 1.5f, 2.0f};
        tfFocused.padding = {2.0f, 4.0f, 2.0f, 4.0f};
        tfFocused.isSet = true;
        styles_[static_cast<int>(WidgetTag::TextField)][static_cast<int>(PseudoState::Focused)] = tfFocused;

        // Slider
        Style slider;
        slider.background = {50, 50, 50, 255};
        slider.border = {{70, 70, 70, 255}, 1.0f, 4.0f};
        slider.isSet = true;
        styles_[static_cast<int>(WidgetTag::Slider)][static_cast<int>(PseudoState::Normal)] = slider;

        // Checkbox
        Style cb;
        cb.background = {40, 40, 40, 255};
        cb.border = {{80, 80, 80, 255}, 1.0f, 2.0f};
        cb.isSet = true;
        styles_[static_cast<int>(WidgetTag::Checkbox)][static_cast<int>(PseudoState::Normal)] = cb;

        // ScrollView
        Style sv;
        sv.background = {35, 35, 35, 255};
        sv.isSet = true;
        styles_[static_cast<int>(WidgetTag::ScrollView)][static_cast<int>(PseudoState::Normal)] = sv;

        // Separator
        Style sep;
        sep.background = {60, 60, 60, 255};
        sep.isSet = true;
        styles_[static_cast<int>(WidgetTag::Separator)][static_cast<int>(PseudoState::Normal)] = sep;

        // TreeNode
        Style tn;
        tn.background = {0, 0, 0, 0};
        tn.textColor = {210, 210, 210, 255};
        tn.padding = {2.0f, 4.0f, 2.0f, 4.0f};
        tn.isSet = true;
        styles_[static_cast<int>(WidgetTag::TreeNode)][static_cast<int>(PseudoState::Normal)] = tn;

        Style tnHover;
        tnHover.background = {50, 50, 60, 255};
        tnHover.textColor = {255, 255, 255, 255};
        tnHover.padding = {2.0f, 4.0f, 2.0f, 4.0f};
        tnHover.isSet = true;
        styles_[static_cast<int>(WidgetTag::TreeNode)][static_cast<int>(PseudoState::Hovered)] = tnHover;

        Style tnSelected;
        tnSelected.background = {35, 65, 115, 255};
        tnSelected.textColor = {255, 255, 255, 255};
        tnSelected.padding = {2.0f, 4.0f, 2.0f, 4.0f};
        tnSelected.isSet = true;
        styles_[static_cast<int>(WidgetTag::TreeNode)][static_cast<int>(PseudoState::Selected)] = tnSelected;

        // DockContainer
        Style dock;
        dock.background = {25, 25, 25, 255};
        dock.isSet = true;
        styles_[static_cast<int>(WidgetTag::DockContainer)][static_cast<int>(PseudoState::Normal)] = dock;

        // SplitPane
        Style split;
        split.background = {35, 35, 35, 255};
        split.isSet = true;
        styles_[static_cast<int>(WidgetTag::SplitPane)][static_cast<int>(PseudoState::Normal)] = split;

        // TabBar
        Style tab;
        tab.background = {45, 45, 45, 255};
        tab.textColor = {200, 200, 200, 255};
        tab.isSet = true;
        styles_[static_cast<int>(WidgetTag::TabBar)][static_cast<int>(PseudoState::Normal)] = tab;

        // PopupMenu
        Style popup;
        popup.background = {50, 50, 55, 255};
        popup.border = {{80, 80, 80, 255}, 1.0f, 4.0f};
        popup.isSet = true;
        styles_[static_cast<int>(WidgetTag::PopupMenu)][static_cast<int>(PseudoState::Normal)] = popup;

        // MenuItem
        Style mi;
        mi.background = {0, 0, 0, 0};
        mi.textColor = {210, 210, 210, 255};
        mi.padding = {2.0f, 6.0f, 2.0f, 6.0f};
        mi.isSet = true;
        styles_[static_cast<int>(WidgetTag::MenuItem)][static_cast<int>(PseudoState::Normal)] = mi;

        Style miHover;
        miHover.background = {60, 100, 180, 255};
        miHover.textColor = {255, 255, 255, 255};
        miHover.padding = {2.0f, 6.0f, 2.0f, 6.0f};
        miHover.isSet = true;
        styles_[static_cast<int>(WidgetTag::MenuItem)][static_cast<int>(PseudoState::Hovered)] = miHover;

        // FloatingPanel
        Style fp;
        fp.background = {30, 30, 38, 245};
        fp.textColor = {210, 210, 220, 255};
        fp.fontSize = 12.0f;
        fp.isSet = true;
        styles_[static_cast<int>(WidgetTag::FloatingPanel)][static_cast<int>(PseudoState::Normal)] = fp;

        // Disabled state defaults for all types
        for (int t = 0; t < TAG_COUNT; ++t) {
            Style& normal = styles_[t][static_cast<int>(PseudoState::Normal)];
            Style disabled = normal;
            disabled.opacity = 0.4f;
            disabled.isSet = true;
            styles_[t][static_cast<int>(PseudoState::Disabled)] = disabled;
        }
    }

    Style styles_[TAG_COUNT][STATE_COUNT]{};
    std::unordered_map<int, std::array<Style, STATE_COUNT>> widgetStyles_;
    mutable std::unordered_map<int, TransitionState> transitions_;

    static Color4 LerpColor(Color4 a, Color4 b, float t) {
        return {
            static_cast<uint8_t>(a.r + (b.r - a.r) * t),
            static_cast<uint8_t>(a.g + (b.g - a.g) * t),
            static_cast<uint8_t>(a.b + (b.b - a.b) * t),
            static_cast<uint8_t>(a.a + (b.a - a.a) * t)
        };
    }

    static float Lerp(float a, float b, float t) { return a + (b - a) * t; }

    static Style LerpStyle(const Style& a, const Style& b, float t) {
        Style r = b; // start from target for non-interpolated fields
        r.background = LerpColor(a.background, b.background, t);
        r.textColor = LerpColor(a.textColor, b.textColor, t);
        r.caretColor = LerpColor(a.caretColor, b.caretColor, t);
        r.accentColor = LerpColor(a.accentColor, b.accentColor, t);
        r.placeholderColor = LerpColor(a.placeholderColor, b.placeholderColor, t);
        r.border.color = LerpColor(a.border.color, b.border.color, t);
        r.border.width = Lerp(a.border.width, b.border.width, t);
        r.border.radiusTL = Lerp(a.border.radiusTL, b.border.radiusTL, t);
        r.border.radiusTR = Lerp(a.border.radiusTR, b.border.radiusTR, t);
        r.border.radiusBR = Lerp(a.border.radiusBR, b.border.radiusBR, t);
        r.border.radiusBL = Lerp(a.border.radiusBL, b.border.radiusBL, t);
        r.opacity = Lerp(a.opacity, b.opacity, t);
        return r;
    }

    static bool ColorEqual(Color4 a, Color4 b) {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
    }

    static bool StyleEqual(const Style& a, const Style& b) {
        return ColorEqual(a.background, b.background) &&
               ColorEqual(a.textColor, b.textColor) &&
               ColorEqual(a.caretColor, b.caretColor) &&
               ColorEqual(a.accentColor, b.accentColor) &&
               ColorEqual(a.placeholderColor, b.placeholderColor) &&
               ColorEqual(a.border.color, b.border.color) &&
               a.border.width == b.border.width &&
               a.border.radiusTL == b.border.radiusTL &&
               a.border.radiusTR == b.border.radiusTR &&
               a.border.radiusBR == b.border.radiusBR &&
               a.border.radiusBL == b.border.radiusBL &&
               a.opacity == b.opacity;
    }

    KL_BEGIN_FIELDS(Theme)
        /* No simple reflected fields - styles_ is a 2D array. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Theme)
        KL_METHOD_AUTO(Theme, Set, "Set"),
        KL_METHOD_AUTO(Theme, Resolve, "Resolve")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Theme)
        KL_CTOR0(Theme)
    KL_END_DESCRIBE(Theme)

};

} // namespace ui
} // namespace koilo
