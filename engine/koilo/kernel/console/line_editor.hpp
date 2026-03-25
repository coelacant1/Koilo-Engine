// SPDX-License-Identifier: GPL-3.0-or-later
/// @file line_editor.hpp
/// @brief Terminal line editor with history, cursor movement, and tab completion.
#pragma once

#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <termios.h>

namespace koilo {

/// Minimal readline-style line editor using POSIX termios.
///
/// Features: cursor movement (left/right, Home/End), history (up/down),
/// word delete (Ctrl+W), line clear (Ctrl+U), screen clear (Ctrl+L),
/// tab completion callback.
class LineEditor {
public:
    using CompleteFunc = std::function<std::vector<std::string>(const std::string&)>;

    LineEditor();
    ~LineEditor();

    /// Read one line from stdin with editing support.
    /// Returns false on EOF (Ctrl+D on empty line).
    bool ReadLine(const std::string& prompt, std::string& out);

    /// Provide read-only access to external history (e.g. ConsoleSession).
    void SetHistory(const std::deque<std::string>* history) { history_ = history; }

    /// Set tab-completion callback.
    void SetCompleter(CompleteFunc fn) { completer_ = std::move(fn); }

private:
    void EnableRawMode();
    void DisableRawMode();
    void RefreshLine(const std::string& prompt);
    void HandleTab(const std::string& prompt);
    void ClearScreen(const std::string& prompt);

    std::string buf_;       ///< Current edit buffer.
    size_t      cursor_;    ///< Cursor position in buf_.
    int         histIdx_;   ///< -1 = current input, 0..N = history index from end.
    std::string saved_;     ///< Saved current input when browsing history.

    const std::deque<std::string>* history_ = nullptr;
    CompleteFunc completer_;

    bool rawModeActive_ = false;
    struct termios origTermios_;
};

} // namespace koilo
