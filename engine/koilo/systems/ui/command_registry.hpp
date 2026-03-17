// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file command_registry.hpp
 * @brief Central registry mapping command IDs to callables, with keyboard
 *        shortcut bindings and dispatch.
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "event.hpp"
#include "../../registry/reflect_macros.hpp"

namespace koilo {
namespace ui {

/** @class Shortcut
 *  @brief A keyboard shortcut: modifier flags + key.
 */
struct Shortcut {
    KeyCode key = KeyCode::None; ///< Key code for this shortcut.
    uint8_t ctrl  : 1; ///< Ctrl modifier flag.
    uint8_t shift : 1; ///< Shift modifier flag.
    uint8_t alt   : 1; ///< Alt modifier flag.
    uint8_t super : 1; ///< Super (OS/Meta) modifier flag.
    uint8_t pad   : 4; ///< Padding bits.

    constexpr Shortcut()
        : key(KeyCode::None), ctrl(0), shift(0), alt(0), super(0), pad(0) {}

    constexpr Shortcut(KeyCode k, bool c = false, bool s = false,
                       bool a = false, bool su = false)
        : key(k), ctrl(c), shift(s), alt(a), super(su), pad(0) {}

    /** @brief Check equality with another shortcut. */
    bool operator==(const Shortcut& o) const {
        return key == o.key && ctrl == o.ctrl && shift == o.shift
            && alt == o.alt && super == o.super;
    }

    /** @brief Return true if the shortcut has a valid key assigned. */
    bool IsValid() const { return key != KeyCode::None; }

    /** @brief Match against an Event's key + modifiers.
     *  @param k  Key code from the event.
     *  @param m  Modifier state from the event.
     *  @return True if this shortcut matches the given key and modifiers.
     */
    bool Matches(KeyCode k, const Modifiers& m) const {
        return key == k && ctrl == m.ctrl && shift == m.shift
            && alt == m.alt && super == m.super;
    }

    /** @brief Get human-readable label (e.g. "Ctrl+Shift+Z"). */
    std::string Label() const;

    /** @brief Get the display name for a KeyCode. */
    static const char* KeyName(KeyCode k);

    KL_BEGIN_FIELDS(Shortcut)
        KL_FIELD(Shortcut, key, "Key", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Shortcut)
        KL_METHOD_AUTO(Shortcut, IsValid, "Is valid"),
        KL_METHOD_AUTO(Shortcut, Matches, "Matches"),
        KL_METHOD_AUTO(Shortcut, Label, "Label")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Shortcut)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Shortcut)

};

/** @class CommandEntry
 *  @brief Registered command entry.
 */
struct CommandEntry {
    std::string id;          ///< Unique string identifier (e.g. "edit.undo")
    std::string label;       ///< Human-readable label (e.g. "Undo")
    std::string category;    ///< Category for grouping (e.g. "Edit")
    Shortcut shortcut;       ///< Default keyboard shortcut
    std::function<void()> execute; ///< The action to perform
    std::function<bool()> canExecute; ///< Optional predicate (nullptr = always enabled)

    KL_BEGIN_FIELDS(CommandEntry)
        KL_FIELD(CommandEntry, id, "Id", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(CommandEntry)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CommandEntry)
        /* No reflected ctors. */
    KL_END_DESCRIBE(CommandEntry)

};

/** @class CommandRegistry
 *  @brief Central command registry: stores named commands and dispatches shortcuts.
 */
class CommandRegistry {
public:
    /** @brief Register a command. Overwrites if id already exists.
     *  @param id         Unique string identifier (e.g. "edit.undo").
     *  @param label      Human-readable label.
     *  @param category   Category for grouping (e.g. "Edit").
     *  @param execute    The action to perform.
     *  @param canExecute Optional predicate (nullptr = always enabled).
     *  @param shortcut   Default keyboard shortcut.
     */
    void Register(const std::string& id, const std::string& label,
                  const std::string& category,
                  std::function<void()> execute,
                  std::function<bool()> canExecute = nullptr,
                  Shortcut shortcut = {}) {
        commands_[id] = {id, label, category, shortcut,
                         std::move(execute), std::move(canExecute)};
        if (shortcut.IsValid()) {
            shortcutMap_[ShortcutKey(shortcut)] = id;
        }
    }

    /** @brief Execute a command by id.
     *  @param id  Command identifier.
     *  @return False if not found or disabled.
     */
    bool Execute(const std::string& id);

    /** @brief Check if a command is currently enabled.
     *  @param id  Command identifier.
     */
    bool IsEnabled(const std::string& id) const {
        auto it = commands_.find(id);
        if (it == commands_.end()) return false;
        if (it->second.canExecute) return it->second.canExecute();
        return true;
    }

    /** @brief Bind (or rebind) a shortcut to a command.
     *  @param id        Command identifier.
     *  @param shortcut  New shortcut to assign.
     */
    void BindShortcut(const std::string& id, Shortcut shortcut);

    /** @brief Try to dispatch a key event as a shortcut.
     *  @return True if a matching command was found and executed.
     */
    bool DispatchShortcut(KeyCode key, const Modifiers& mods) {
        Shortcut s(key, mods.ctrl, mods.shift, mods.alt, mods.super);
        auto it = shortcutMap_.find(ShortcutKey(s));
        if (it == shortcutMap_.end()) return false;
        return Execute(it->second);
    }

    /** @brief Look up a command entry (nullptr if not found).
     *  @param id  Command identifier.
     */
    const CommandEntry* Find(const std::string& id) const {
        auto it = commands_.find(id);
        return it != commands_.end() ? &it->second : nullptr;
    }

    /** @brief Get the shortcut label for a command (empty if none).
     *  @param id  Command identifier.
     */
    std::string ShortcutLabel(const std::string& id) const {
        auto it = commands_.find(id);
        if (it == commands_.end() || !it->second.shortcut.IsValid()) return {};
        return it->second.shortcut.Label();
    }

    /** @brief Iterate all commands (for command palette / settings UI). */
    const std::unordered_map<std::string, CommandEntry>& All() const {
        return commands_;
    }

    /** @brief Get commands grouped by category. */
    std::vector<std::pair<std::string, std::vector<const CommandEntry*>>>
    ByCategory() const;

private:
    /// Packs a Shortcut into a single uint32 for hash-map lookup.
    struct ShortcutKey {
        uint32_t value; ///< Packed key + modifier bits.
        explicit ShortcutKey(const Shortcut& s)
            : value(static_cast<uint32_t>(s.key)
                    | (static_cast<uint32_t>(s.ctrl)  << 16)
                    | (static_cast<uint32_t>(s.shift) << 17)
                    | (static_cast<uint32_t>(s.alt)   << 18)
                    | (static_cast<uint32_t>(s.super) << 19)) {}
        bool operator==(const ShortcutKey& o) const { return value == o.value; }

        KL_BEGIN_FIELDS(ShortcutKey)
            KL_FIELD(ShortcutKey, value, "Value", 0, 4294967295)
        KL_END_FIELDS

        KL_BEGIN_METHODS(ShortcutKey)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(ShortcutKey)
            /* No reflected ctors. */
        KL_END_DESCRIBE(ShortcutKey)

    };
    /// Hash functor for ShortcutKey.
    struct ShortcutKeyHash {
        size_t operator()(const ShortcutKey& k) const { return std::hash<uint32_t>{}(k.value); }

        KL_BEGIN_FIELDS(ShortcutKeyHash)
            /* No reflected fields. */
        KL_END_FIELDS

        KL_BEGIN_METHODS(ShortcutKeyHash)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(ShortcutKeyHash)
            /* No reflected ctors. */
        KL_END_DESCRIBE(ShortcutKeyHash)

    };

    std::unordered_map<std::string, CommandEntry> commands_; ///< All registered commands by id.
    std::unordered_map<ShortcutKey, std::string, ShortcutKeyHash> shortcutMap_; ///< Shortcut -> command id.

    KL_BEGIN_FIELDS(CommandRegistry)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(CommandRegistry)
        KL_METHOD_AUTO(CommandRegistry, ShortcutLabel, "Shortcut label"),
        KL_METHOD_AUTO(CommandRegistry, ByCategory, "By category")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CommandRegistry)
        /* No reflected ctors. */
    KL_END_DESCRIBE(CommandRegistry)

};

} // namespace ui
} // namespace koilo
