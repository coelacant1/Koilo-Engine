// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file console_widget.hpp
 * @brief In-engine developer console overlay widget.
 *
 * Builds a floating panel with a scrollable output area and command
 * input field using the Koilo UI system. Routes commands through the
 * kernel ConsoleSession and displays results in the output area.
 * Toggle visibility with F12 or programmatically via Toggle().
 */

#pragma once

#include <koilo/core/interfaces/iconsole_widget.hpp>
#include <koilo/kernel/console/console_session.hpp>
#include <koilo/kernel/console/command_registry.hpp>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace koilo {

class KoiloKernel;
class UI;

namespace ui {
class UIContext;
} // namespace ui

/**
 * @class ConsoleWidget
 * @brief In-engine console overlay backed by UIContext widgets.
 *
 * Creates a floating panel containing a scrollable log area and a
 * text input field. Commands typed into the input are dispatched
 * through the kernel ConsoleSession and results are appended as
 * label widgets in the scroll view.
 *
 * The widget does not own the UI or kernel - the caller must ensure
 * they outlive this object.
 */
class ConsoleWidget : public IConsoleWidget {
public:
    ConsoleWidget() = default;

    /**
     * @brief Build the console overlay inside the given UI.
     * @param ui       UI instance to create widgets in.
     * @param kernel   Kernel for command execution context.
     * @param registry Command registry for dispatching commands.
     * @param x        Initial X position of the floating panel.
     * @param y        Initial Y position of the floating panel.
     * @param w        Initial width of the floating panel.
     * @param h        Initial height of the floating panel.
     */
    void Build(UI& ui, KoiloKernel& kernel, CommandRegistry& registry,
               float x = 50.0f, float y = 50.0f,
               float w = 700.0f, float h = 350.0f);

    /** @brief Show the console overlay. */
    void Show();
    /** @brief Hide the console overlay. */
    void Hide();
    /** @brief Toggle console visibility. */
    void Toggle() override;
    /** @brief Check if the console is visible. */
    bool IsVisible() const override { return visible_; }

    /**
     * @brief Submit the current text field contents as a command.
     *
     * Call this when the user presses Return in the input field.
     * The command is executed via ConsoleSession and the result is
     * appended to the output area.
     */
    void SubmitInput() override;

    /**
     * @brief Append a line of text to the console output.
     * @param text     Text to display.
     * @param isError  If true, style as error output.
     */
    void AppendOutput(const std::string& text, bool isError = false);

    /** @brief Clear all output from the console. */
    void ClearOutput() override;

    /**
     * @brief Per-frame update. Checks for pending input submission.
     *
     * Call this each frame from the module Tick. It monitors the
     * input field for Return key presses (via the UI onChange
     * callback mechanism).
     */
    void Update() override;

    /** @brief Get the floating panel widget index. */
    int GetPanelIndex() const { return panelIdx_; }
    /** @brief Get the input field widget index. */
    int GetInputIndex() const { return inputIdx_; }

    /** @brief Check if Build() has been called successfully. */
    bool IsBuilt() const override { return panelIdx_ >= 0; }

private:
    void ScrollToBottom();

    UI* ui_ = nullptr;
    KoiloKernel* kernel_ = nullptr;
    std::unique_ptr<ConsoleSession> session_;

    int panelIdx_   = -1;   // Floating panel
    int scrollIdx_  = -1;   // ScrollView for output
    int logIdx_     = -1;   // Panel inside scroll view (holds labels)
    int inputIdx_   = -1;   // TextField for command input

    bool visible_ = false;
    uint32_t lineCount_ = 0;
    static constexpr uint32_t kMaxLines = 512;

    std::string pendingSubmit_;
    bool hasPendingSubmit_ = false;
};

} // namespace koilo
