// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui.hpp
 * @brief Scripting-facing UI wrapper around UIContext.
 *
 * Provides the registered "UI" type for KoiloScript. Delegates to the
 * retained-mode UIContext widget system for all operations.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include "ui_context.hpp"
#include "ui_animation.hpp"
#include "localization.hpp"
#include "render/draw_list.hpp"
#ifdef KL_HAVE_OPENGL_BACKEND
#include "render/ui_gl_renderer.hpp"
#endif
#include "render/ui_sw_renderer.hpp"
#include "markup/kml_loader.hpp"
#include <koilo/systems/font/font.hpp>
#include <koilo/core/color/color888.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <chrono>
#include <memory>

#ifdef KL_HAVE_VULKAN_BACKEND
#include "render/ui_vk_renderer.hpp"
#endif

namespace koilo {

/**
 * @class UI
 * @brief Scripting-facing UI manager wrapping ui::UIContext.
 */
class UI {
public:
    /** @brief Default constructor. Initializes root widget. */
    UI();
    /** @brief Destructor. */
    ~UI() = default;

    /// Access the underlying UIContext.
    ui::UIContext& Context() { return ctx_; }
    const ui::UIContext& Context() const { return ctx_; }

    // -- Viewport & layout ---------------------------------------
    /** @brief Set the viewport dimensions. */
    void SetViewport(float w, float h);
    /** @brief Run the layout pass. */
    void UpdateLayout();

    // -- Widget creation -----------------------------------------
    /** @brief Create a panel widget. */
    int CreatePanel(const char* id);
    /** @brief Create a label widget. */
    int CreateLabel(const char* id, const char* text);
    /** @brief Create a button widget. */
    int CreateButton(const char* id, const char* text);
    /** @brief Create a slider widget. */
    int CreateSlider(const char* id, float min, float max, float value);
    /** @brief Create a checkbox widget. */
    int CreateCheckbox(const char* id, bool checked);
    /** @brief Create a text field widget. */
    int CreateTextField(const char* id, const char* placeholder);
    /** @brief Create a separator widget. */
    int CreateSeparator(const char* id);
    /** @brief Create a scroll view widget. */
    int CreateScrollView(const char* id);
    /** @brief Create a tree node widget. */
    int CreateTreeNode(const char* id, const char* text);
    /** @brief Create a dock container widget. */
    int CreateDockContainer(const char* id);
    /** @brief Create a split pane widget. */
    int CreateSplitPane(const char* id, bool vertical);
    /** @brief Create a tab bar widget. */
    int CreateTabBar(const char* id);
    /** @brief Create a popup menu widget. */
    int CreatePopupMenu(const char* id);
    /** @brief Create a menu item widget. */
    int CreateMenuItem(const char* id, const char* text);

    // -- Widget tree ---------------------------------------------
    /** @brief Set widget parent-child relationship. */
    bool SetParent(int child, int parent);
    /** @brief Destroy a widget by index. */
    void DestroyWidget(int idx);
    /** @brief Find a widget by string ID. */
    int FindWidget(const char* id);
    /** @brief Get the root widget index. */
    int GetRoot();
    /** @brief Get the number of live widgets. */
    int GetWidgetCount();

    // -- Properties ----------------------------------------------
    /** @brief Set the text of a widget. */
    void SetText(int idx, const char* text);
    /** @brief Get the text of a widget. */
    const char* GetText(int idx);
    /** @brief Set widget visibility. */
    void SetVisible(int idx, bool v);
    /** @brief Set widget enabled state. */
    void SetEnabled(int idx, bool e);
    /** @brief Set widget selected state. */
    void SetSelected(int idx, bool s);
    /** @brief Set a ColorField's color from a hex string (e.g. "#FF8800"). */
    void SetColorHex(int idx, const char* hex);
    /** @brief Set widget size. */
    void SetSize(int idx, float w, float h);
    /** @brief Set widget position. */
    void SetPosition(int idx, float x, float y);
    /** @brief Set widget padding (top, right, bottom, left). */
    void SetPadding(int idx, float top, float right, float bottom, float left);
    /** @brief Set widget margin (top, right, bottom, left). */
    void SetMargin(int idx, float top, float right, float bottom, float left);

    // -- Layout --------------------------------------------------
    /** @brief Set column layout with spacing. */
    void SetLayoutColumn(int idx, float spacing);
    /** @brief Set row layout with spacing. */
    void SetLayoutRow(int idx, float spacing);
    /** @brief Set widget to fill available width. */
    void SetFillWidth(int idx);
    /** @brief Set widget to fill available height. */
    void SetFillHeight(int idx);

    // -- Events (script callbacks) -------------------------------
    /** @brief Set click callback script function name. */
    void SetOnClick(int idx, const char* fnName);
    /** @brief Set change callback script function name. */
    void SetOnChange(int idx, const char* fnName);

    // -- Inspector -----------------------------------------------
    /// Auto-generate inspector UI for a registered type by class name.
    /// Returns root widget index or -1 on failure.
    int InspectType(const char* className, int parentIdx);

    // -- Theme ---------------------------------------------------
    /** @brief Override theme color for a widget element type. */
    void SetThemeColor(const char* element, int r, int g, int b, int a);

    // -- Query ---------------------------------------------------
    /** @brief Get the current slider value. */
    float GetSliderValue(int idx);
    /** @brief Get checkbox checked state. */
    bool GetChecked(int idx);
    /** @brief Get viewport width. */
    float GetViewportWidth();
    /** @brief Get viewport height. */
    float GetViewportHeight();

    /// Render UI to a Color888 buffer (software path).
    void RenderToBuffer(Color888* buffer, int width, int height);

    /// Initialize GPU UI rendering (OpenGL). Call once after GL context is ready.
#ifdef KL_HAVE_OPENGL_BACKEND
    bool InitializeGPU();

    /// Render UI overlay via OpenGL. Call after scene rendering, before swap.
    void RenderGPU(int viewportW, int viewportH);
#endif

#ifdef KL_HAVE_VULKAN_BACKEND
    /// Initialize Vulkan UI rendering. Call once after Vulkan device is ready.
    bool InitializeVulkanGPU(VulkanBackend* backend);

    /// Render UI overlay via Vulkan into the active command buffer.
    void RenderVulkanGPU(int viewportW, int viewportH, VkCommandBuffer cmd);

    /// Shut down Vulkan UI resources. Must be called before VkDevice is destroyed.
    void ShutdownVulkanGPU();
#endif

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
    void HandleKeyDown(int keyCode, bool shift = false, bool ctrl = false, bool alt = false);
    void HandleTextInput(const char* text);

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

    /// Check if the UI needs visual updates (state changed or transitions active).
    bool IsIdle() const;

private:
    ui::UIContext ctx_;              ///< Retained-mode widget context.
    ui::TweenPool tweenPool_{256};   ///< Pool for active UI tweens.
    Localization localization_;      ///< Localization string table.
    ui::UIDrawList drawList_;        ///< Intermediate draw command list.
#ifdef KL_HAVE_OPENGL_BACKEND
    ui::UIGLRenderer glRenderer_;    ///< OpenGL UI renderer.
#endif
    ui::UISWRenderer swRenderer_;    ///< Software UI renderer.
#ifdef KL_HAVE_VULKAN_BACKEND
    std::unique_ptr<ui::UIVkRenderer> vkRenderer_;  ///< Vulkan UI renderer.
    bool vkGpuInitialized_ = false;
    bool vkGpuInitFailed_ = false;
#endif
    font::Font font_;                ///< Loaded TTF font for text rendering.
    uint32_t fontAtlasTexture_ = 0;  ///< GPU texture handle for font atlas.
    bool gpuInitialized_ = false;    ///< Whether GPU renderer has been initialized.
    std::chrono::steady_clock::time_point lastFrameTime_ = std::chrono::steady_clock::now(); ///< Last frame timestamp for dt calculation.

    /// Apply tween value to the target widget property.
    static void ApplyTween(int widgetIdx, ui::TweenProperty prop,
                           float value, void* userData);

    /// Pre-layout pass: auto-size text widgets that haven't been given explicit sizes.
    void AutoSizeTextWidgets();

    /// Append debug overlay text (watched CVars) to the draw list.
    void AppendDebugOverlay(int viewportW, int viewportH);

    KL_DECLARE_FIELDS(UI)
    KL_DECLARE_METHODS(UI)
    KL_DECLARE_DESCRIBE(UI)


};

} // namespace koilo
