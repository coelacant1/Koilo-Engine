// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file style.hpp
 * @brief Style properties, pseudo-states, and theme system for the UI engine.
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include "widget.hpp"
#include "../../registry/reflect_macros.hpp"
#include <koilo/core/math/mathematics.hpp>
#include <unordered_map>
#include <array>

namespace koilo {
namespace ui {

// --- Pseudo-States ---

/// Interaction states that drive style resolution.
enum class PseudoState : uint8_t {
    Normal = 0,
    Hovered,       ///< Mouse is over the widget.
    Pressed,       ///< Mouse button held down on the widget.
    Focused,       ///< Widget has input focus.
    FocusVisible,  ///< Keyboard-only focus (Tab navigation).
    Selected,      ///< Widget is in the selected state.
    Disabled,      ///< Widget is disabled and non-interactive.
    Count          ///< Sentinel - total number of pseudo-states.
};

// --- Style ---

/**
 * @class BorderStyle
 * @brief Border appearance including color, width, and per-corner radii.
 */
struct BorderStyle {
    Color4 color{80, 80, 80, 255}; ///< Border color.
    float width = 0.0f;           ///< Border width in pixels.
    float radiusTL = 0.0f;        ///< Top-left corner radius.
    float radiusTR = 0.0f;        ///< Top-right corner radius.
    float radiusBR = 0.0f;        ///< Bottom-right corner radius.
    float radiusBL = 0.0f;        ///< Bottom-left corner radius.

    /** @brief Set all corners to the same radius. */
    void SetRadius(float r) { radiusTL = radiusTR = radiusBR = radiusBL = r; }

    /**
     * @brief Set per-corner radii (CSS order: TL TR BR BL).
     * @param tl Top-left radius.
     * @param tr Top-right radius.
     * @param br Bottom-right radius.
     * @param bl Bottom-left radius.
     */
    void SetRadius(float tl, float tr, float br, float bl) {
        radiusTL = tl; radiusTR = tr; radiusBR = br; radiusBL = bl;
    }

    /** @brief True if any corner has a non-zero radius. */
    bool HasRadius() const { return radiusTL > 0 || radiusTR > 0 || radiusBR > 0 || radiusBL > 0; }
    /** @brief True if all corners share the same radius. */
    bool IsUniform() const { return radiusTL == radiusTR && radiusTR == radiusBR && radiusBR == radiusBL; }
    /** @brief Get uniform radius (returns TL; valid when IsUniform()). */
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

/**
 * @class Shadow
 * @brief Box shadow effect parameters.
 */
struct Shadow {
    float offsetX = 0.0f;          ///< Horizontal shadow offset.
    float offsetY = 0.0f;          ///< Vertical shadow offset.
    float blur = 0.0f;             ///< Blur radius.
    float spread = 0.0f;           ///< Spread distance.
    Color4 color{0, 0, 0, 128};   ///< Shadow color.
    bool active = false;           ///< Whether this shadow is enabled.

    KL_BEGIN_FIELDS(Shadow)
        KL_FIELD(Shadow, offsetX, "Offset x", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Shadow)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Shadow)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Shadow)

};

/**
 * @class TextShadow
 * @brief Text shadow effect parameters.
 */
struct TextShadow {
    float offsetX = 0.0f;          ///< Horizontal shadow offset.
    float offsetY = 0.0f;          ///< Vertical shadow offset.
    float blur = 0.0f;             ///< Blur radius.
    Color4 color{0, 0, 0, 128};   ///< Shadow color.
    bool active = false;           ///< Whether this text shadow is enabled.

    KL_BEGIN_FIELDS(TextShadow)
        KL_FIELD(TextShadow, offsetX, "Offset x", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(TextShadow)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(TextShadow)
        /* No reflected ctors. */
    KL_END_DESCRIBE(TextShadow)

};

/**
 * @class Gradient
 * @brief Linear gradient background parameters.
 */
struct Gradient {
    Color4 from{0, 0, 0, 0};      ///< Start color.
    Color4 to{0, 0, 0, 0};        ///< End color.
    float angle = 180.0f;          ///< Angle in degrees (180 = top-to-bottom).
    bool active = false;           ///< Whether this gradient is enabled.

    KL_BEGIN_FIELDS(Gradient)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Gradient)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Gradient)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Gradient)

};

/**
 * @class Transition
 * @brief Style transition animation parameters.
 */
struct Transition {
    float duration = 0.0f;         ///< Duration in seconds.
    bool active = false;           ///< Whether transitions are enabled.

    KL_BEGIN_FIELDS(Transition)
        KL_FIELD(Transition, duration, "Duration", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Transition)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Transition)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Transition)

};

/**
 * @class Transform2D
 * @brief 2D transform applied to a widget.
 */
struct Transform2D {
    float rotate = 0.0f;          ///< Rotation in degrees.
    float scaleX = 1.0f;          ///< Horizontal scale factor.
    float scaleY = 1.0f;          ///< Vertical scale factor.
    float translateX = 0.0f;      ///< Horizontal translation in pixels.
    float translateY = 0.0f;      ///< Vertical translation in pixels.
    float originX = 0.5f;         ///< Transform origin X (0.0 = left, 1.0 = right).
    float originY = 0.5f;         ///< Transform origin Y (0.0 = top, 1.0 = bottom).
    bool active = false;          ///< Whether this transform is enabled.

    /** @brief True if the transform is the identity (no effect). */
    bool IsIdentity() const {
        return rotate == 0.0f && scaleX == 1.0f && scaleY == 1.0f &&
               translateX == 0.0f && translateY == 0.0f;
    }

    KL_BEGIN_FIELDS(Transform2D)
        KL_FIELD(Transform2D, rotate, "Rotate", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Transform2D)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Transform2D)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Transform2D)

};

/**
 * @class Style
 * @brief Visual appearance properties for a single widget state.
 */
struct Style {
    Color4 background{40, 40, 40, 255};           ///< Background fill color.
    Color4 textColor{220, 220, 220, 255};          ///< Text color.
    Color4 caretColor{220, 220, 220, 255};         ///< Text cursor color.
    Color4 accentColor{80, 140, 200, 255};         ///< Checkbox/slider accent color.
    Color4 placeholderColor{120, 120, 140, 180};   ///< Placeholder text color.
    BorderStyle border{};                          ///< Border appearance.
    float fontSize = 14.0f;                        ///< Font size in pixels.
    float opacity = 1.0f;                          ///< Opacity (0.0-1.0).
    Edges padding{0.0f, 0.0f, 0.0f, 0.0f};        ///< Inner padding.
    Shadow shadow{};                               ///< Box shadow effect.
    TextShadow textShadow{};                       ///< Text shadow effect.
    Gradient gradient{};                           ///< Gradient background.
    Transition transition{};                       ///< Transition animation params.
    Transform2D transform{};                       ///< 2D transform.
    bool isSet = false;                            ///< True if explicitly set, not inherited.

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

/**
 * @class Theme
 * @brief Holds one Style per (WidgetTag, PseudoState) combination.
 */
class Theme {
public:
    static constexpr int TAG_COUNT   = static_cast<int>(WidgetTag::Count);   ///< Number of widget tags.
    static constexpr int STATE_COUNT = static_cast<int>(PseudoState::Count); ///< Number of pseudo-states.

    /** @brief Construct theme with default dark styles. */
    Theme() { SetDefaults(); }

    /**
     * @brief Set the style for a given tag and pseudo-state.
     * @param tag   Widget tag to configure.
     * @param state Pseudo-state to set.
     * @param style Style to store.
     */
    void Set(WidgetTag tag, PseudoState state, const Style& style) {
        auto& s = styles_[static_cast<int>(tag)][static_cast<int>(state)];
        s = style;
        s.isSet = true;
    }

    /**
     * @brief Get the style for a given tag and pseudo-state.
     * @param tag   Widget tag to look up.
     * @param state Pseudo-state to query.
     * @return Const reference to the matching style, falling back to Normal.
     */
    const Style& Get(WidgetTag tag, PseudoState state) const {
        const auto& s = styles_[static_cast<int>(tag)][static_cast<int>(state)];
        if (s.isSet) return s;
        // Fall back to Normal state
        return styles_[static_cast<int>(tag)][static_cast<int>(PseudoState::Normal)];
    }

    /**
     * @brief Set a per-widget style override (KSS-computed, not global theme).
     * @param widgetIdx Widget pool index.
     * @param state     Pseudo-state to set.
     * @param style     Style to store.
     */
    void SetWidget(int widgetIdx, PseudoState state, const Style& style) {
        auto& arr = widgetStyles_[widgetIdx];
        auto& s = arr[static_cast<int>(state)];
        s = style;
        s.isSet = true;
    }

    /**
     * @brief Get per-widget style override.
     * @param widgetIdx Widget pool index.
     * @param state     Pseudo-state to query.
     * @return Pointer to the override style, or nullptr if none set.
     */
    const Style* GetWidget(int widgetIdx, PseudoState state) const;

    /**
     * @brief Resolve the effective style for a widget based on its current state.
     * @param w         Widget to resolve for.
     * @param widgetIdx Pool index (-1 to skip per-widget overrides).
     * @return Resolved style.
     */
    Style Resolve(const Widget& w, int widgetIdx = -1) const;

    /** @brief Clear all per-widget styles (called before re-applying stylesheet). */
    void ClearWidgetStyles() { widgetStyles_.clear(); }

    /**
     * @class TransitionState
     * @brief Transition animation state for a single widget.
     */
    struct TransitionState {
        Style current;      ///< Currently rendered (interpolated) style.
        Style target;       ///< Target style from current pseudo-state.
        float remaining;    ///< Seconds remaining in transition.
        float duration;     ///< Total transition duration.
        bool active = false; ///< Whether this transition is in progress.

        KL_BEGIN_FIELDS(TransitionState)
            KL_FIELD(TransitionState, current, "Current", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(TransitionState)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(TransitionState)
            /* No reflected ctors. */
        KL_END_DESCRIBE(TransitionState)

    };

    /** @brief Update transitions for all widgets. Call once per frame with delta time. */
    void UpdateTransitions(float dt);

    /** @brief True if any widget has an in-progress transition animation. */
    bool HasActiveTransitions() const { return !transitions_.empty(); }

    /**
     * @brief Resolve with transition: returns interpolated style if transitioning.
     * @param w         Widget to resolve for.
     * @param widgetIdx Pool index (-1 to skip per-widget overrides).
     * @return Animated style.
     */
    Style ResolveAnimated(const Widget& w, int widgetIdx = -1) const;

    /**
     * @brief Notify that a widget's state changed (triggers transition if configured).
     * @param w         Widget whose state changed.
     * @param widgetIdx Pool index.
     */
    void NotifyStateChange(const Widget& w, int widgetIdx);

private:
    void SetDefaults();

    Style styles_[TAG_COUNT][STATE_COUNT]{};                               ///< Global theme style table.
    std::unordered_map<int, std::array<Style, STATE_COUNT>> widgetStyles_; ///< Per-widget style overrides.
    mutable std::unordered_map<int, TransitionState> transitions_;         ///< Active transition states.

    static Color4 LerpColor(Color4 a, Color4 b, float t) {
        return {
            static_cast<uint8_t>(Mathematics::Lerp(static_cast<float>(a.r), static_cast<float>(b.r), t)),
            static_cast<uint8_t>(Mathematics::Lerp(static_cast<float>(a.g), static_cast<float>(b.g), t)),
            static_cast<uint8_t>(Mathematics::Lerp(static_cast<float>(a.b), static_cast<float>(b.b), t)),
            static_cast<uint8_t>(Mathematics::Lerp(static_cast<float>(a.a), static_cast<float>(b.a), t))
        };
    }

    static Style LerpStyle(const Style& a, const Style& b, float t);

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
