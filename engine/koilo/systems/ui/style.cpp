// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file style.cpp
 * @brief Theme system implementation - style resolution, transitions, defaults.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "style.hpp"

namespace koilo {
namespace ui {

// ============================================================================
// Style Lookup
// ============================================================================

// Get per-widget style override, returns nullptr if none set.
const Style* Theme::GetWidget(int widgetIdx, PseudoState state) const {
    auto it = widgetStyles_.find(widgetIdx);
    if (it == widgetStyles_.end()) return nullptr;
    const auto& s = it->second[static_cast<int>(state)];
    if (s.isSet) return &s;
    const auto& normal = it->second[static_cast<int>(PseudoState::Normal)];
    if (normal.isSet) return &normal;
    return nullptr;
}

// Resolve the effective style for a widget based on its current state.
Style Theme::Resolve(const Widget& w, int widgetIdx) const {
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

// ============================================================================
// Transitions
// ============================================================================

// Update transitions for all widgets.
void Theme::UpdateTransitions(float dt) {
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
            float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
            ts.current = LerpStyle(ts.current, ts.target, ease);
            ++it;
        }
    }
}

// Resolve with transition: returns interpolated style if transitioning.
Style Theme::ResolveAnimated(const Widget& w, int widgetIdx) const {
    Style target = Resolve(w, widgetIdx);
    if (widgetIdx < 0) return target;

    float duration = target.transition.active ? target.transition.duration : 0.0f;
    if (duration <= 0.0f) {
        transitions_.erase(widgetIdx);
        return target;
    }

    auto it = transitions_.find(widgetIdx);
    if (it != transitions_.end()) {
        auto& ts = it->second;
        if (!StyleEqual(ts.target, target)) {
            ts.target = target;
            ts.remaining = duration;
            ts.duration = duration;
            ts.active = true;
        }
        return ts.current;
    } else {
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

// Notify that a widget's state changed (triggers transition if configured).
void Theme::NotifyStateChange(const Widget& w, int widgetIdx) {
    Style target = Resolve(w, widgetIdx);
    float duration = target.transition.active ? target.transition.duration : 0.0f;
    if (duration <= 0.0f) return;

    auto it = transitions_.find(widgetIdx);
    if (it != transitions_.end()) {
        auto& ts = it->second;
        if (!StyleEqual(ts.target, target)) {
            ts.target = target;
            ts.remaining = duration;
            ts.duration = duration;
            ts.active = true;
        }
    } else {
        TransitionState ts;
        ts.current = target;
        ts.target = target;
        ts.remaining = 0.0f;
        ts.duration = duration;
        ts.active = false;
        transitions_[widgetIdx] = ts;
    }
}

// ============================================================================
// Style Interpolation
// ============================================================================

// Interpolate between two styles at factor t.
Style Theme::LerpStyle(const Style& a, const Style& b, float t) {
    Style r = b;
    r.background = LerpColor(a.background, b.background, t);
    r.textColor = LerpColor(a.textColor, b.textColor, t);
    r.caretColor = LerpColor(a.caretColor, b.caretColor, t);
    r.accentColor = LerpColor(a.accentColor, b.accentColor, t);
    r.placeholderColor = LerpColor(a.placeholderColor, b.placeholderColor, t);
    r.border.color = LerpColor(a.border.color, b.border.color, t);
    r.border.width = Mathematics::Lerp(a.border.width, b.border.width, t);
    r.border.radiusTL = Mathematics::Lerp(a.border.radiusTL, b.border.radiusTL, t);
    r.border.radiusTR = Mathematics::Lerp(a.border.radiusTR, b.border.radiusTR, t);
    r.border.radiusBR = Mathematics::Lerp(a.border.radiusBR, b.border.radiusBR, t);
    r.border.radiusBL = Mathematics::Lerp(a.border.radiusBL, b.border.radiusBL, t);
    r.opacity = Mathematics::Lerp(a.opacity, b.opacity, t);
    return r;
}

// ============================================================================
// Default Theme
// ============================================================================

// Initialize the default dark theme for all widget types.
void Theme::SetDefaults() {
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

    // VirtualList
    Style vl;
    vl.background = {0, 0, 0, 0};
    vl.isSet = true;
    styles_[static_cast<int>(WidgetTag::VirtualList)][static_cast<int>(PseudoState::Normal)] = vl;

    // Canvas2D
    Style c2d;
    c2d.background = {0, 0, 0, 0};
    c2d.isSet = true;
    styles_[static_cast<int>(WidgetTag::Canvas2D)][static_cast<int>(PseudoState::Normal)] = c2d;

    // Disabled state defaults for all types
    for (int t = 0; t < TAG_COUNT; ++t) {
        Style& normal = styles_[t][static_cast<int>(PseudoState::Normal)];
        Style disabled = normal;
        disabled.opacity = 0.4f;
        disabled.isSet = true;
        styles_[t][static_cast<int>(PseudoState::Disabled)] = disabled;
    }
}

} // namespace ui
} // namespace koilo
