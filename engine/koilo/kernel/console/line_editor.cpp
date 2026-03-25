// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/console/line_editor.hpp>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <algorithm>

namespace koilo {

// -- Terminal helpers --------------------------------------------------------

LineEditor::LineEditor() = default;

LineEditor::~LineEditor() {
    if (rawModeActive_) DisableRawMode();
}

void LineEditor::EnableRawMode() {
    if (rawModeActive_) return;
    if (!isatty(STDIN_FILENO)) return;

    tcgetattr(STDIN_FILENO, &origTermios_);
    struct termios raw = origTermios_;

    // Input: no break, no CR-to-NL, no parity, no strip, no flow control
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // Output: leave alone (we still want \n -> \r\n from printf)
    // Local: no echo, no canonical, no signals via Ctrl+C (we handle SIGINT)
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    // Read returns after 1 byte, no timeout
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    rawModeActive_ = true;
}

void LineEditor::DisableRawMode() {
    if (!rawModeActive_) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios_);
    rawModeActive_ = false;
}

// -- Display -----------------------------------------------------------------

void LineEditor::RefreshLine(const std::string& prompt) {
    // Move cursor to start of line, clear, redraw
    std::string seq;
    seq += '\r';                              // carriage return
    seq += "\x1b[0K";                         // clear to end of line
    seq += prompt;
    seq += buf_;
    // Position cursor
    size_t cursorCol = prompt.size() + cursor_;
    seq += '\r';
    if (cursorCol > 0) {
        char moveBuf[32];
        snprintf(moveBuf, sizeof(moveBuf), "\x1b[%zuC", cursorCol);
        seq += moveBuf;
    }
    write(STDOUT_FILENO, seq.data(), seq.size());
}

void LineEditor::ClearScreen(const std::string& prompt) {
    write(STDOUT_FILENO, "\x1b[H\x1b[2J", 7); // home + clear
    RefreshLine(prompt);
}

// -- Tab completion ----------------------------------------------------------

void LineEditor::HandleTab(const std::string& prompt) {
    if (!completer_) return;

    auto candidates = completer_(buf_);
    if (candidates.empty()) return;

    if (candidates.size() == 1) {
        // Single match: complete it
        buf_ = candidates[0];
        if (!buf_.empty() && buf_.back() != ' ') buf_ += ' ';
        cursor_ = buf_.size();
        RefreshLine(prompt);
        return;
    }

    // Find longest common prefix
    std::string prefix = candidates[0];
    for (size_t i = 1; i < candidates.size(); ++i) {
        size_t j = 0;
        while (j < prefix.size() && j < candidates[i].size()
               && prefix[j] == candidates[i][j])
            ++j;
        prefix.resize(j);
    }

    if (prefix.size() > buf_.size()) {
        buf_ = prefix;
        cursor_ = buf_.size();
        RefreshLine(prompt);
        return;
    }

    // Show candidates
    const char* nl = "\r\n";
    write(STDOUT_FILENO, nl, 2);
    for (auto& c : candidates) {
        write(STDOUT_FILENO, c.data(), c.size());
        write(STDOUT_FILENO, "  ", 2);
    }
    write(STDOUT_FILENO, nl, 2);
    RefreshLine(prompt);
}

// -- Main read loop ----------------------------------------------------------

bool LineEditor::ReadLine(const std::string& prompt, std::string& out) {
    buf_.clear();
    cursor_ = 0;
    histIdx_ = -1;
    saved_.clear();

    if (!isatty(STDIN_FILENO)) {
        // Fallback for pipes: plain getline
        printf("%s", prompt.c_str());
        fflush(stdout);
        if (!std::getline(std::cin, out)) return false;
        return true;
    }

    EnableRawMode();

    // Print prompt
    write(STDOUT_FILENO, prompt.data(), prompt.size());

    while (true) {
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n <= 0) {
            // EOF
            DisableRawMode();
            write(STDOUT_FILENO, "\r\n", 2);
            return false;
        }

        switch (c) {
        case '\r': // Enter
        case '\n':
            DisableRawMode();
            write(STDOUT_FILENO, "\r\n", 2);
            out = buf_;
            return true;

        case 4: // Ctrl+D
            if (buf_.empty()) {
                DisableRawMode();
                write(STDOUT_FILENO, "\r\n", 2);
                return false;
            }
            // Non-empty: delete char under cursor
            if (cursor_ < buf_.size()) {
                buf_.erase(cursor_, 1);
                RefreshLine(prompt);
            }
            break;

        case 127: // Backspace (most terminals)
        case 8:   // Ctrl+H
            if (cursor_ > 0) {
                buf_.erase(--cursor_, 1);
                RefreshLine(prompt);
            }
            break;

        case 1: // Ctrl+A  home
            cursor_ = 0;
            RefreshLine(prompt);
            break;

        case 5: // Ctrl+E  end
            cursor_ = buf_.size();
            RefreshLine(prompt);
            break;

        case 11: // Ctrl+K  kill to end of line
            buf_.erase(cursor_);
            RefreshLine(prompt);
            break;

        case 21: // Ctrl+U  clear line
            buf_.clear();
            cursor_ = 0;
            RefreshLine(prompt);
            break;

        case 23: { // Ctrl+W  delete word back
            if (cursor_ == 0) break;
            size_t pos = cursor_;
            while (pos > 0 && buf_[pos - 1] == ' ') --pos;
            while (pos > 0 && buf_[pos - 1] != ' ') --pos;
            buf_.erase(pos, cursor_ - pos);
            cursor_ = pos;
            RefreshLine(prompt);
            break;
        }

        case 12: // Ctrl+L  clear screen
            ClearScreen(prompt);
            break;

        case '\t': // Tab
            HandleTab(prompt);
            break;

        case 27: { // Escape sequence
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) break;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) break;

            if (seq[0] == '[') {
                switch (seq[1]) {
                case 'A': { // Up arrow - older history
                    if (!history_ || history_->empty()) break;
                    int maxIdx = static_cast<int>(history_->size()) - 1;
                    if (histIdx_ < maxIdx) {
                        if (histIdx_ == -1) saved_ = buf_;
                        ++histIdx_;
                        buf_ = (*history_)[history_->size() - 1 - histIdx_];
                        cursor_ = buf_.size();
                        RefreshLine(prompt);
                    }
                    break;
                }
                case 'B': { // Down arrow - newer history
                    if (histIdx_ > 0) {
                        --histIdx_;
                        buf_ = (*history_)[history_->size() - 1 - histIdx_];
                        cursor_ = buf_.size();
                        RefreshLine(prompt);
                    } else if (histIdx_ == 0) {
                        histIdx_ = -1;
                        buf_ = saved_;
                        cursor_ = buf_.size();
                        RefreshLine(prompt);
                    }
                    break;
                }
                case 'C': // Right arrow
                    if (cursor_ < buf_.size()) {
                        ++cursor_;
                        RefreshLine(prompt);
                    }
                    break;
                case 'D': // Left arrow
                    if (cursor_ > 0) {
                        --cursor_;
                        RefreshLine(prompt);
                    }
                    break;
                case 'H': // Home
                    cursor_ = 0;
                    RefreshLine(prompt);
                    break;
                case 'F': // End
                    cursor_ = buf_.size();
                    RefreshLine(prompt);
                    break;
                case '3': { // Delete key (ESC [ 3 ~)
                    char tilde;
                    if (read(STDIN_FILENO, &tilde, 1) > 0 && tilde == '~') {
                        if (cursor_ < buf_.size()) {
                            buf_.erase(cursor_, 1);
                            RefreshLine(prompt);
                        }
                    }
                    break;
                }
                case '1': // Home (ESC [ 1 ~) or xterm (ESC [ 1 ; ...)
                case '7': { // Home (rxvt)
                    char tilde;
                    if (read(STDIN_FILENO, &tilde, 1) > 0 && tilde == '~') {
                        cursor_ = 0;
                        RefreshLine(prompt);
                    }
                    break;
                }
                case '4': // End (ESC [ 4 ~)
                case '8': { // End (rxvt)
                    char tilde;
                    if (read(STDIN_FILENO, &tilde, 1) > 0 && tilde == '~') {
                        cursor_ = buf_.size();
                        RefreshLine(prompt);
                    }
                    break;
                }
                default:
                    break;
                }
            } else if (seq[0] == 'O') {
                // ESC O H = Home, ESC O F = End (xterm application mode)
                if (seq[1] == 'H') { cursor_ = 0; RefreshLine(prompt); }
                if (seq[1] == 'F') { cursor_ = buf_.size(); RefreshLine(prompt); }
            }
            break;
        }

        default:
            // Printable character
            if (c >= 32) {
                buf_.insert(buf_.begin() + cursor_, c);
                ++cursor_;
                RefreshLine(prompt);
            }
            break;
        }
    }
}

} // namespace koilo
