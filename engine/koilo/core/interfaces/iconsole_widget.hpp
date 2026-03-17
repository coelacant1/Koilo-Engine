// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace koilo {

/// Minimal interface for console widget interaction from the kernel layer.
/// Implemented by ConsoleWidget in systems/ui/. Used by ConsoleModule to
/// avoid a direct include dependency on the UI subsystem.
class IConsoleWidget {
public:
    virtual ~IConsoleWidget() = default;

    /// Whether the widget has been built into the UI.
    virtual bool IsBuilt() const = 0;

    /// Whether the widget is currently visible.
    virtual bool IsVisible() const = 0;

    /// Toggle visibility of the console widget.
    virtual void Toggle() = 0;

    /// Submit the current input line for execution.
    virtual void SubmitInput() = 0;

    /// Clear all output lines.
    virtual void ClearOutput() = 0;

    /// Per-frame update (cursor blink, scroll, etc.).
    virtual void Update() = 0;
};

} // namespace koilo
