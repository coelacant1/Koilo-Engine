// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file console_widget.cpp
 * @brief In-engine developer console overlay widget implementation.
 */

#include "console_widget.hpp"
#include <koilo/kernel/kernel.hpp>
#include <koilo/systems/ui/ui.hpp>
#include <koilo/systems/ui/ui_context.hpp>
#include <koilo/systems/ui/widget.hpp>
#include <sstream>

namespace koilo {

// ---------------------------------------------------------
// Build
// ---------------------------------------------------------

void ConsoleWidget::Build(UI& ui, KoiloKernel& kernel,
                          CommandRegistry& registry,
                          float x, float y, float w, float h) {
    ui_ = &ui;
    kernel_ = &kernel;

    // Create a ConsoleSession with output routed to the widget
    session_ = std::make_unique<ConsoleSession>(kernel, registry);
    session_->SetOutputCallback([this](const std::string& text) {
        AppendOutput(text);
    });

    auto& ctx = ui.Context();

    // Floating panel
    panelIdx_ = ctx.CreateFloatingPanel("__dev_console", "Developer Console",
                                         x, y, w, h);
    // High z-order so it floats above game content
    if (auto* pw = ctx.GetWidget(panelIdx_)) {
        pw->zOrder = 900;
    }

    // Body container (column layout: scroll + input)
    int body = ui.CreatePanel("__dev_console_body");
    ui.SetParent(body, panelIdx_);
    ui.SetFillWidth(body);
    ui.SetFillHeight(body);
    ui.SetLayoutColumn(body, 0);
    ui.SetPadding(body, 2, 4, 2, 4);

    // Scrollable output area (takes most of the panel height)
    scrollIdx_ = ui.CreateScrollView("__dev_console_scroll");
    ui.SetParent(scrollIdx_, body);
    ui.SetFillWidth(scrollIdx_);
    ui.SetFillHeight(scrollIdx_);

    // Log container inside scroll view
    logIdx_ = ui.CreatePanel("__dev_console_log");
    ui.SetParent(logIdx_, scrollIdx_);
    ui.SetFillWidth(logIdx_);
    ui.SetLayoutColumn(logIdx_, 1);

    // Welcome message
    AppendOutput("Koilo Developer Console v0.1.0");
    AppendOutput("Type 'help' for available commands.");
    AppendOutput("");

    // Input field at the bottom
    inputIdx_ = ui.CreateTextField("__dev_console_input", "Enter command...");
    ui.SetParent(inputIdx_, body);
    ui.SetFillWidth(inputIdx_);
    ui.SetSize(inputIdx_, 0, 24);

    // Start hidden
    ui.SetVisible(panelIdx_, false);
    visible_ = false;
}

// ---------------------------------------------------------
// Visibility
// ---------------------------------------------------------

void ConsoleWidget::Show() {
    if (!ui_ || panelIdx_ < 0) return;
    ui_->SetVisible(panelIdx_, true);
    visible_ = true;

    // Focus the input field
    auto& ctx = ui_->Context();
    auto* inputW = ctx.GetWidget(inputIdx_);
    if (inputW) {
        inputW->flags.focused = 1;
    }
}

void ConsoleWidget::Hide() {
    if (!ui_ || panelIdx_ < 0) return;
    ui_->SetVisible(panelIdx_, false);
    visible_ = false;
}

void ConsoleWidget::Toggle() {
    if (visible_) Hide();
    else Show();
}

// ---------------------------------------------------------
// Input / Output
// ---------------------------------------------------------

void ConsoleWidget::SubmitInput() {
    if (!ui_ || !session_ || inputIdx_ < 0) return;

    const char* text = ui_->GetText(inputIdx_);
    if (!text || text[0] == '\0') return;

    std::string command(text);

    // Echo the command in the output
    AppendOutput("> " + command);

    // Execute through session
    auto result = session_->Execute(command);

    // Display the result
    if (!result.text.empty()) {
        AppendOutput(result.text, !result.ok());
    }

    // Clear input field
    ui_->SetText(inputIdx_, "");

    // Reset cursor
    auto& ctx = ui_->Context();
    auto* inputW = ctx.GetWidget(inputIdx_);
    if (inputW) {
        inputW->cursorPos = 0;
    }

    ScrollToBottom();
}

void ConsoleWidget::AppendOutput(const std::string& text, bool isError) {
    if (!ui_ || logIdx_ < 0) return;

    // Split multi-line text into individual labels
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        // Generate a unique ID for each label
        std::string labelId = "__dcl_" + std::to_string(lineCount_);

        int label = ui_->CreateLabel(labelId.c_str(),
                                     line.empty() ? " " : line.c_str());
        ui_->SetParent(label, logIdx_);
        ui_->SetFillWidth(label);
        ui_->SetSize(label, 0, 16);

        // Style error lines differently (red-ish via font weight)
        if (isError) {
            auto& ctx = ui_->Context();
            auto* w = ctx.GetWidget(label);
            if (w) {
                w->fontWeight = 700; // Bold for errors
            }
        }

        lineCount_++;

        // Trim old lines if we exceed the max
        if (lineCount_ > kMaxLines) {
            // Remove the oldest child from the log container
            auto& ctx = ui_->Context();
            auto* logW = ctx.GetWidget(logIdx_);
            if (logW && logW->childCount > 0) {
                int oldest = logW->children[0];
                ctx.DestroyWidget(oldest);
            }
        }
    }
}

void ConsoleWidget::ClearOutput() {
    if (!ui_ || logIdx_ < 0) return;

    auto& ctx = ui_->Context();
    auto* logW = ctx.GetWidget(logIdx_);
    if (!logW) return;

    // Destroy all child labels
    while (logW->childCount > 0) {
        int child = logW->children[0];
        ctx.DestroyWidget(child);
        // Re-fetch since DestroyWidget may compact children
        logW = ctx.GetWidget(logIdx_);
        if (!logW) break;
    }

    lineCount_ = 0;
}

void ConsoleWidget::Update() {
    if (!visible_ || !ui_ || inputIdx_ < 0) return;

    // Process any pending submit
    if (hasPendingSubmit_) {
        hasPendingSubmit_ = false;
        SubmitInput();
    }
}

void ConsoleWidget::ScrollToBottom() {
    if (!ui_ || scrollIdx_ < 0) return;

    auto& ctx = ui_->Context();
    auto* scrollW = ctx.GetWidget(scrollIdx_);
    if (scrollW) {
        scrollW->scrollY = scrollW->contentHeight;
        scrollW->flags.dirty = 1;
    }
}

} // namespace koilo
