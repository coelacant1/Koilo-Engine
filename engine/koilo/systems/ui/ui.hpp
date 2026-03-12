// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui.hpp
 * @brief Scripting-facing UI wrapper around UIContext.
 *
 * Provides the registered "UI" type for KoiloScript. Delegates to the
 * retained-mode UIContext widget system for all operations.
 *
 * @date 03/08/2026
 * @author Coela
 */

#pragma once

#include "ui_context.hpp"
#include "ui_animation.hpp"
#include "localization.hpp"
#include "render/draw_list.hpp"
#include "render/ui_gl_renderer.hpp"
#include "render/ui_sw_renderer.hpp"
#include "markup/kml_loader.hpp"
#include <koilo/systems/font/font.hpp>
#include <koilo/core/color/color888.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <chrono>

namespace koilo {

/**
 * @class UI
 * @brief Scripting-facing UI manager wrapping ui::UIContext.
 */
class UI {
public:
    UI();
    ~UI() = default;

    /// Access the underlying UIContext.
    ui::UIContext& Context() { return ctx_; }
    const ui::UIContext& Context() const { return ctx_; }

    // -- Viewport & layout ---------------------------------------
    void SetViewport(float w, float h);
    void UpdateLayout();

    // -- Widget creation -----------------------------------------
    int CreatePanel(const char* id);
    int CreateLabel(const char* id, const char* text);
    int CreateButton(const char* id, const char* text);
    int CreateSlider(const char* id, float min, float max, float value);
    int CreateCheckbox(const char* id, bool checked);
    int CreateTextField(const char* id, const char* placeholder);
    int CreateSeparator(const char* id);
    int CreateScrollView(const char* id);
    int CreateTreeNode(const char* id, const char* text);
    int CreateDockContainer(const char* id);
    int CreateSplitPane(const char* id, bool vertical);
    int CreateTabBar(const char* id);
    int CreatePopupMenu(const char* id);
    int CreateMenuItem(const char* id, const char* text);

    // -- Widget tree ---------------------------------------------
    bool SetParent(int child, int parent);
    void DestroyWidget(int idx);
    int FindWidget(const char* id);
    int GetRoot();
    int GetWidgetCount();

    // -- Properties ----------------------------------------------
    void SetText(int idx, const char* text);
    const char* GetText(int idx);
    void SetVisible(int idx, bool v);
    void SetEnabled(int idx, bool e);
    void SetSize(int idx, float w, float h);
    void SetPosition(int idx, float x, float y);
    void SetPadding(int idx, float top, float right, float bottom, float left);
    void SetMargin(int idx, float top, float right, float bottom, float left);

    // -- Layout --------------------------------------------------
    void SetLayoutColumn(int idx, float spacing);
    void SetLayoutRow(int idx, float spacing);
    void SetFillWidth(int idx);
    void SetFillHeight(int idx);

    // -- Events (script callbacks) -------------------------------
    void SetOnClick(int idx, const char* fnName);
    void SetOnChange(int idx, const char* fnName);

    // -- Inspector -----------------------------------------------
    /// Auto-generate inspector UI for a registered type by class name.
    /// Returns root widget index or -1 on failure.
    int InspectType(const char* className, int parentIdx);

    // -- Theme ---------------------------------------------------
    void SetThemeColor(const char* element, int r, int g, int b, int a);

    // -- Query ---------------------------------------------------
    float GetSliderValue(int idx);
    bool GetChecked(int idx);
    float GetViewportWidth();
    float GetViewportHeight();

    /// Render UI to a Color888 buffer (software path).
    void RenderToBuffer(Color888* buffer, int width, int height);

    /// Initialize GPU UI rendering (OpenGL). Call once after GL context is ready.
    bool InitializeGPU();

    /// Render UI overlay via OpenGL. Call after scene rendering, before swap.
    void RenderGPU(int viewportW, int viewportH);

    /// Load a TTF font for UI text rendering.
    bool LoadFont(const char* path, float pixelSize);

    /// Check if a font is loaded.
    bool HasFont() const;

    /// Remove all widgets and reset state.
    void Clear();

    // -- Markup loading (KML + KSS) ----------------------------

    /// Load a UI layout from KML markup + optional KSS stylesheet.
    /// Returns root widget index, or -1 on failure.
    int LoadMarkup(const char* kmlPath, const char* kssPath);

    /// Load a UI layout from KML/KSS strings (in-memory).
    int LoadMarkupString(const char* kml, const char* kss);

    // -- Input events (from host) --------------------------------
    /// Forward a pointer (mouse) event to the UI for hit testing and dispatch.
    /// button: 0=left, 1=right, 2=middle
    void HandlePointerDown(float x, float y, uint8_t button = 0);
    void HandlePointerUp(float x, float y, uint8_t button = 0);
    void HandlePointerMove(float x, float y);
    void HandleScroll(float x, float y, float delta);

    /// Get the cursor type requested by the currently hovered widget.
    ui::CursorType GetRequestedCursor() const;

    // -- Animation -----------------------------------------------
    /// Start a tween. Returns tween index.
    int Animate(int widgetIdx, const char* property, float from, float to,
                float duration, const char* easing);

    /// Cancel a tween by index.
    void CancelAnimation(int tweenIdx);

    /// Cancel all tweens on a widget.
    void CancelAllAnimations(int widgetIdx);

    /// Update all active animations. Call once per frame.
    void UpdateAnimations(float dt);

    // -- Localization --------------------------------------------
    /// Set active locale string (e.g., "en", "ja").
    void SetLocale(const char* locale);

    /// Look up a localized string by key.
    const char* L(const char* key);

    /// Add a localized string entry.
    void SetLocalizedString(const char* key, const char* value);

    // -- Accessibility -------------------------------------------
    /// Set screen reader label for a widget.
    void SetAriaLabel(int idx, const char* label);

    /// Get screen reader label for a widget.
    const char* GetAriaLabel(int idx);

    /// Set global font scale multiplier.
    void SetFontScale(float scale);

    /// Get global font scale multiplier.
    float GetFontScale();

private:
    ui::UIContext ctx_;
    ui::TweenPool tweenPool_{256};
    Localization localization_;
    ui::UIDrawList drawList_;
    ui::UIGLRenderer glRenderer_;
    ui::UISWRenderer swRenderer_;
    font::Font font_;
    uint32_t fontAtlasTexture_ = 0;
    bool gpuInitialized_ = false;
    std::chrono::steady_clock::time_point lastFrameTime_ = std::chrono::steady_clock::now();

    /// Apply tween value to the target widget property.
    static void ApplyTween(int widgetIdx, ui::TweenProperty prop,
                           float value, void* userData);

    /// Pre-layout pass: auto-size text widgets that haven't been given explicit sizes.
    void AutoSizeTextWidgets();

    KL_DECLARE_FIELDS(UI)
    KL_DECLARE_METHODS(UI)
    KL_DECLARE_DESCRIBE(UI)

};

} // namespace koilo
