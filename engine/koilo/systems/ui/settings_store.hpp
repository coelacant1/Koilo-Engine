// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file settings_store.hpp
 * @brief Key-value settings persistence for the Koilo editor.
 *
 * Provides typed Get/Set with defaults, serialization to a simple
 * text format, and category-based grouping. No external dependencies.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/ui/widget.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <fstream>

namespace koilo {
namespace ui {

/// Supported setting value types.
using SettingValue = std::variant<bool, int, float, std::string, Color4>;

/** @class SettingEntry @brief A single setting entry with metadata. */
struct SettingEntry {
    std::string key;            ///< Unique setting identifier
    std::string category;       ///< Category for grouping in the preferences UI
    std::string label;          ///< Human-readable label
    SettingValue value;         ///< Current setting value
    SettingValue defaultValue;  ///< Default value for reset
};

/** @class SettingsStore @brief Persistent key-value settings store with serialization. */
class SettingsStore {
public:
    // -- Typed accessors -------------------------------------------------

    /** @brief Get a typed setting value by key. */
    template<typename T>
    T Get(const std::string& key, const T& defaultVal = T{}) const {
        auto it = entries_.find(key);
        if (it == entries_.end()) return defaultVal;
        const auto* v = std::get_if<T>(&it->second.value);
        return v ? *v : defaultVal;
    }

    /** @brief Set a typed setting value by key. */
    template<typename T>
    void Set(const std::string& key, const T& val) {
        auto it = entries_.find(key);
        if (it != entries_.end()) {
            it->second.value = val;
        } else {
            entries_[key] = {key, "", key, val, val};
        }
        dirty_ = true;
    }

    /** @brief Check whether a setting key exists. */
    bool Has(const std::string& key) const {
        return entries_.find(key) != entries_.end();
    }

    /** @brief Define a setting with category/label/default for the preferences UI. */
    template<typename T>
    void Define(const std::string& key, const std::string& category,
                const std::string& label, const T& defaultVal) {
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            entries_[key] = {key, category, label, defaultVal, defaultVal};
        } else {
            it->second.category = category;
            it->second.label = label;
            it->second.defaultValue = defaultVal;
        }
    }

    /** @brief Reset a single setting to its default. */
    void ResetToDefault(const std::string& key) {
        auto it = entries_.find(key);
        if (it != entries_.end()) {
            it->second.value = it->second.defaultValue;
            dirty_ = true;
        }
    }

    /** @brief Reset all settings to defaults. */
    void ResetAllDefaults() {
        for (auto& [k, e] : entries_) {
            e.value = e.defaultValue;
        }
        dirty_ = true;
    }

    /** @brief Check whether any settings have been modified. */
    bool IsDirty() const { return dirty_; }
    /** @brief Clear the dirty flag after saving. */
    void MarkClean() { dirty_ = false; }

    // -- Category queries ------------------------------------------------

    /** @brief Collect unique category names from all entries. */
    std::vector<std::string> Categories() const;

    /** @brief Get all setting entries belonging to a category. */
    std::vector<const SettingEntry*> EntriesInCategory(const std::string& cat) const {
        std::vector<const SettingEntry*> result;
        for (const auto& [k, e] : entries_) {
            if (e.category == cat) result.push_back(&e);
        }
        return result;
    }

    /** @brief Get a reference to all setting entries. */
    const std::unordered_map<std::string, SettingEntry>& AllEntries() const {
        return entries_;
    }

    // -- Serialization ---------------------------------------------------

    /** @brief Serialize all settings to a string in key=value format. */
    std::string Serialize() const;

    /**
     * @brief Deserialize settings from key=value format string.
     * Only updates keys that already exist (defined via Define or Set).
     */
    void Deserialize(const std::string& data);

    /** @brief Save to file. @return True on success. */
    bool SaveToFile(const std::string& path) {
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << Serialize();
        dirty_ = false;
        return true;
    }

    /** @brief Load from file. @return True on success. */
    bool LoadFromFile(const std::string& path);

private:
    std::unordered_map<std::string, SettingEntry> entries_; ///< All registered settings
    bool dirty_ = false;                                    ///< True when settings changed since last save

    static std::string Trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        return s.substr(start, end - start + 1);
    }

    static SettingValue ParseAutoValue(const std::string& val);

    static Color4 ParseColor(const std::string& hex);

    static void ParseInto(SettingValue& target, const std::string& val);
};

} // namespace ui
} // namespace koilo
