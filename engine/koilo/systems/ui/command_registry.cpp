// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file command_registry.cpp
 * @brief Command registry implementation.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "command_registry.hpp"

namespace koilo {
namespace ui {

// ============================================================================
// Shortcut
// ============================================================================

// Human-readable shortcut label (e.g. "Ctrl+Shift+Z").
std::string Shortcut::Label() const {
    std::string s;
    if (ctrl)  s += "Ctrl+";
    if (shift) s += "Shift+";
    if (alt)   s += "Alt+";
    if (super) s += "Super+";
    s += KeyName(key);
    return s;
}

// Map a KeyCode to its display name.
const char* Shortcut::KeyName(KeyCode k) {
    switch (k) {
        case KeyCode::Tab:       return "Tab";
        case KeyCode::Escape:    return "Esc";
        case KeyCode::Return:    return "Enter";
        case KeyCode::Backspace: return "Backspace";
        case KeyCode::Delete:    return "Del";
        case KeyCode::Space:     return "Space";
        case KeyCode::Left:      return "Left";
        case KeyCode::Right:     return "Right";
        case KeyCode::Up:        return "Up";
        case KeyCode::Down:      return "Down";
        case KeyCode::Home:      return "Home";
        case KeyCode::End:       return "End";
        case KeyCode::PageUp:    return "PgUp";
        case KeyCode::PageDown:  return "PgDn";
        case KeyCode::Minus:     return "-";
        case KeyCode::Plus:      return "+";
        case KeyCode::Period:    return ".";
        case KeyCode::Comma:     return ",";
        case KeyCode::F1:  return "F1";  case KeyCode::F2:  return "F2";
        case KeyCode::F3:  return "F3";  case KeyCode::F4:  return "F4";
        case KeyCode::F5:  return "F5";  case KeyCode::F6:  return "F6";
        case KeyCode::F7:  return "F7";  case KeyCode::F8:  return "F8";
        case KeyCode::F9:  return "F9";  case KeyCode::F10: return "F10";
        case KeyCode::F11: return "F11"; case KeyCode::F12: return "F12";
        default: {
            // A-Z or 0-9
            auto v = static_cast<uint16_t>(k);
            auto a = static_cast<uint16_t>(KeyCode::A);
            auto z = static_cast<uint16_t>(KeyCode::Z);
            auto n0 = static_cast<uint16_t>(KeyCode::Num0);
            auto n9 = static_cast<uint16_t>(KeyCode::Num9);
            if (v >= a && v <= z) {
                static char buf[2] = {};
                buf[0] = static_cast<char>('A' + (v - a));
                return buf;
            }
            if (v >= n0 && v <= n9) {
                static char nbuf[2] = {};
                nbuf[0] = static_cast<char>('0' + (v - n0));
                return nbuf;
            }
            return "?";
        }
    }
}

// ============================================================================
// CommandRegistry
// ============================================================================

// Execute a command by id.
bool CommandRegistry::Execute(const std::string& id) {
    auto it = commands_.find(id);
    if (it == commands_.end()) return false;
    auto& cmd = it->second;
    if (cmd.canExecute && !cmd.canExecute()) return false;
    cmd.execute();
    return true;
}

// Bind (or rebind) a shortcut to a command.
void CommandRegistry::BindShortcut(const std::string& id, Shortcut shortcut) {
    auto it = commands_.find(id);
    if (it == commands_.end()) return;

    // Remove old shortcut binding if any
    if (it->second.shortcut.IsValid()) {
        shortcutMap_.erase(ShortcutKey(it->second.shortcut));
    }
    it->second.shortcut = shortcut;
    if (shortcut.IsValid()) {
        shortcutMap_[ShortcutKey(shortcut)] = id;
    }
}

// Get commands grouped by category.
std::vector<std::pair<std::string, std::vector<const CommandEntry*>>>
CommandRegistry::ByCategory() const {
    std::unordered_map<std::string, std::vector<const CommandEntry*>> groups;
    for (auto& [id, cmd] : commands_) {
        groups[cmd.category].push_back(&cmd);
    }
    std::vector<std::pair<std::string, std::vector<const CommandEntry*>>> result;
    result.reserve(groups.size());
    for (auto& [cat, cmds] : groups) {
        result.emplace_back(cat, std::move(cmds));
    }
    return result;
}

} // namespace ui
} // namespace koilo
