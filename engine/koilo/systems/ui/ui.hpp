// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui.hpp
 * @brief UI manager - focus navigation, pointer hit-testing, and overlay rendering.
 *
 * Owns a flat list of widgets and manages focus traversal across focusable
 * widgets. Renders the entire UI tree into a Color888 pixel buffer as a
 * 2D overlay on top of the 3D scene.
 *
 * @date 01/25/2026
 * @author Coela
 */

#pragma once

#include <vector>
#include <string>
#include "widget.hpp"
#include <koilo/systems/input/inputmanager.hpp>

namespace koilo {

/**
 * @class UI
 * @brief Root UI manager - no vtable, reflection-safe.
 *
 * Provides:
 * - Widget ownership and tree management
 * - Focus traversal (NextFocus, PrevFocus, ActivateFocus)
 * - Pointer hit-testing
 * - Overlay rendering to pixel buffer
 */
class UI {
private:
    std::vector<Widget*> widgets_;       // All owned widgets
    std::vector<Widget*> focusList_;     // Focusable widgets in tab order
    int focusIndex_ = -1;               // Current focused widget index (-1 = none)

    std::string activateResult_;        // Last activation callback name

public:
    UI() = default;

    ~UI() {
        for (Widget* w : widgets_) {
            delete w;
        }
        widgets_.clear();
        focusList_.clear();
    }

    // === Widget Management ===

    /**
     * @brief Creates a generic widget and adds it to the UI.
     * @return Pointer to the new widget (owned by UI).
     */
    Widget* CreateWidget() {
        Widget* w = new Widget();
        widgets_.push_back(w);
        return w;
    }

    /**
     * @brief Creates a Label widget.
     */
    Widget* CreateLabel(const std::string& text, float x, float y) {
        Widget* w = koilo::CreateLabel(text, x, y);
        widgets_.push_back(w);
        return w;
    }

    /**
     * @brief Creates a Panel widget.
     */
    Widget* CreatePanel(float x, float y, float w, float h) {
        Widget* widget = koilo::CreatePanel(x, y, w, h);
        widgets_.push_back(widget);
        return widget;
    }

    /**
     * @brief Creates a Button widget.
     */
    Widget* CreateButton(const std::string& text, float x, float y,
                         float w, float h) {
        Widget* widget = koilo::CreateButton(text, x, y, w, h);
        widgets_.push_back(widget);
        return widget;
    }

    /**
     * @brief Adds an externally created widget to the UI.
     * UI takes ownership and will delete it on destruction.
     */
    void AddWidget(Widget* w) {
        if (w) widgets_.push_back(w);
    }

    /**
     * @brief Gets the number of widgets.
     */
    int GetWidgetCount() const { return static_cast<int>(widgets_.size()); }

    /**
     * @brief Gets a widget by index.
     */
    Widget* GetWidget(int index) {
        if (index >= 0 && index < static_cast<int>(widgets_.size()))
            return widgets_[index];
        return nullptr;
    }

    // === Focus Management ===

    /**
     * @brief Rebuilds the focus list from all focusable+visible+enabled widgets.
     * Call after adding/removing widgets or changing focusable state.
     */
    void RebuildFocusList() {
        focusList_.clear();
        CollectFocusable(widgets_);
        focusIndex_ = focusList_.empty() ? -1 : 0;
        UpdateFocusState();
    }

    /**
     * @brief Move focus to the next focusable widget.
     */
    void NextFocus() {
        if (focusList_.empty()) return;
        focusIndex_ = (focusIndex_ + 1) % static_cast<int>(focusList_.size());
        UpdateFocusState();
    }

    /**
     * @brief Move focus to the previous focusable widget.
     */
    void PrevFocus() {
        if (focusList_.empty()) return;
        focusIndex_ = (focusIndex_ - 1 + static_cast<int>(focusList_.size()))
                      % static_cast<int>(focusList_.size());
        UpdateFocusState();
    }

    /**
     * @brief Activate the currently focused widget.
     * @return The onActivate callback name (empty if none).
     */
    std::string ActivateFocus() {
        if (focusIndex_ < 0 || focusIndex_ >= static_cast<int>(focusList_.size()))
            return "";
        Widget* w = focusList_[focusIndex_];
        if (w && w->enabled) {
            activateResult_ = w->onActivate;
            return w->onActivate;
        }
        return "";
    }

    /**
     * @brief Gets the currently focused widget.
     */
    Widget* GetFocusedWidget() {
        if (focusIndex_ >= 0 && focusIndex_ < static_cast<int>(focusList_.size()))
            return focusList_[focusIndex_];
        return nullptr;
    }

    /**
     * @brief Gets the focus list size.
     */
    int GetFocusCount() const { return static_cast<int>(focusList_.size()); }

    /**
     * @brief Gets the last activation callback name.
     */
    const std::string& GetLastActivation() const { return activateResult_; }

    // === Pointer Hit Testing ===

    /**
     * @brief Find the topmost visible widget at the given point.
     * Searches in reverse order (last added = on top).
     */
    Widget* HitTest(float px, float py) {
        // Check children depth-first, reverse order
        for (int i = static_cast<int>(widgets_.size()) - 1; i >= 0; i--) {
            Widget* hit = HitTestRecursive(widgets_[i], px, py);
            if (hit) return hit;
        }
        return nullptr;
    }

    /**
     * @brief Focus the widget at the given point (pointer click).
     * @return The widget that received focus, or nullptr.
     */
    Widget* FocusAtPoint(float px, float py) {
        Widget* hit = HitTest(px, py);
        if (hit && hit->focusable && hit->enabled) {
            // Find in focus list
            for (int i = 0; i < static_cast<int>(focusList_.size()); i++) {
                if (focusList_[i] == hit) {
                    focusIndex_ = i;
                    UpdateFocusState();
                    return hit;
                }
            }
        }
        return nullptr;
    }

    // === Rendering ===

    /**
     * @brief Render all widgets into a pixel buffer.
     * @param buffer Row-major Color888 buffer.
     * @param width Buffer width.
     * @param height Buffer height.
     */
    void RenderToBuffer(Color888* buffer, int width, int height) {
        if (!buffer || width <= 0 || height <= 0) return;

        // Render root-level widgets (they render their children)
        for (Widget* w : widgets_) {
            if (w->parent == nullptr) { // only render root widgets
                w->Render(buffer, width, height);
            }
        }
    }

    // === Input Processing ===

    /**
     * @brief Process input actions for UI navigation.
     * Maps: ui_next -> NextFocus, ui_prev -> PrevFocus, ui_activate -> ActivateFocus
     * @param input InputManager reference.
     * @return Non-empty string if an activation occurred.
     */
    std::string ProcessInput(InputManager& input) {
        if (input.IsActionPressed("ui_next") ||
            input.IsKeyPressed(KeyCode::Tab) ||
            input.IsKeyPressed(KeyCode::Down)) {
            NextFocus();
        }

        if (input.IsActionPressed("ui_prev") ||
            input.IsKeyPressed(KeyCode::Up)) {
            PrevFocus();
        }

        if (input.IsActionPressed("ui_activate") ||
            input.IsKeyPressed(KeyCode::Return) ||
            input.IsKeyPressed(KeyCode::Space)) {
            return ActivateFocus();
        }

        return "";
    }

    // === Cleanup ===

    /**
     * @brief Remove all widgets and reset state.
     */
    void Clear() {
        for (Widget* w : widgets_) delete w;
        widgets_.clear();
        focusList_.clear();
        focusIndex_ = -1;
        activateResult_.clear();
    }

private:
    void CollectFocusable(const std::vector<Widget*>& list) {
        for (Widget* w : list) {
            if (w->focusable && w->visible && w->enabled) {
                focusList_.push_back(w);
            }
            // Also collect from children
            if (!w->children.empty()) {
                CollectFocusable(w->children);
            }
        }
    }

    void UpdateFocusState() {
        for (Widget* w : focusList_) {
            w->focused = false;
        }
        if (focusIndex_ >= 0 && focusIndex_ < static_cast<int>(focusList_.size())) {
            focusList_[focusIndex_]->focused = true;
        }
    }

    Widget* HitTestRecursive(Widget* w, float px, float py) {
        if (!w || !w->visible) return nullptr;
        // Check children first (depth-first, reverse for topmost)
        for (int i = static_cast<int>(w->children.size()) - 1; i >= 0; i--) {
            Widget* hit = HitTestRecursive(w->children[i], px, py);
            if (hit) return hit;
        }
        if (w->Contains(px, py)) return w;
        return nullptr;
    }

public:
    // === Reflection ===
    KL_DECLARE_FIELDS(UI)
    KL_DECLARE_METHODS(UI)
    KL_DECLARE_DESCRIBE(UI)
};

} // namespace koilo
