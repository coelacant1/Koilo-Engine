// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file settings_store.cpp
 * @brief Settings store serialization and persistence implementation.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "settings_store.hpp"

#include <cstring>
#include <sstream>

namespace koilo {
namespace ui {

// ============================================================================
// Category Queries
// ============================================================================

// Collect unique category names from all entries.
std::vector<std::string> SettingsStore::Categories() const {
    std::vector<std::string> cats;
    for (const auto& [k, e] : entries_) {
        bool found = false;
        for (const auto& c : cats)
            if (c == e.category) { found = true; break; }
        if (!found && !e.category.empty()) cats.push_back(e.category);
    }
    return cats;
}

// ============================================================================
// Serialization
// ============================================================================

// Serialize all settings to key=value text.
std::string SettingsStore::Serialize() const {
    std::ostringstream out;
    for (const auto& [k, e] : entries_) {
        out << k << " = ";
        std::visit([&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>) {
                out << (v ? "true" : "false");
            } else if constexpr (std::is_same_v<T, int>) {
                out << v;
            } else if constexpr (std::is_same_v<T, float>) {
                out << v;
            } else if constexpr (std::is_same_v<T, std::string>) {
                out << '"' << v << '"';
            } else if constexpr (std::is_same_v<T, Color4>) {
                char hex[10];
                snprintf(hex, sizeof(hex), "#%02X%02X%02X%02X", v.r, v.g, v.b, v.a);
                out << hex;
            }
        }, e.value);
        out << '\n';
    }
    return out.str();
}

// Deserialize settings from key=value text.
void SettingsStore::Deserialize(const std::string& data) {
    std::istringstream in(data);
    std::string line;
    while (std::getline(in, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = Trim(line.substr(0, eq));
        std::string val = Trim(line.substr(eq + 1));
        if (key.empty()) continue;

        auto it = entries_.find(key);
        if (it == entries_.end()) {
            // Auto-detect type for unknown keys
            entries_[key] = {key, "", key, ParseAutoValue(val), ParseAutoValue(val)};
        } else {
            // Parse value matching existing type
            ParseInto(it->second.value, val);
        }
    }
    dirty_ = false;
}

// ============================================================================
// File I/O
// ============================================================================

// Load settings from a file.
bool SettingsStore::LoadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string data((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());
    Deserialize(data);
    return true;
}

// ============================================================================
// Value Parsing
// ============================================================================

// Auto-detect value type from a raw string.
SettingValue SettingsStore::ParseAutoValue(const std::string& val) {
    if (val == "true") return true;
    if (val == "false") return false;
    if (!val.empty() && val[0] == '"') {
        // String
        return val.substr(1, val.size() > 1 && val.back() == '"' ? val.size() - 2 : val.size() - 1);
    }
    if (!val.empty() && val[0] == '#' && val.size() >= 7) {
        return ParseColor(val);
    }
    // Try int then float
    bool hasDecimal = val.find('.') != std::string::npos;
    if (hasDecimal) {
        try { return std::stof(val); } catch (...) {}
    }
    try { return std::stoi(val); } catch (...) {}
    return std::string(val); // fallback to string
}

// Parse a hex color string (#RRGGBB or #RRGGBBAA).
Color4 SettingsStore::ParseColor(const std::string& hex) {
    const char* p = hex.c_str();
    if (*p == '#') ++p;
    auto h2 = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return 0;
    };
    Color4 c{0, 0, 0, 255};
    if (std::strlen(p) >= 6) {
        c.r = h2(p[0]) * 16 + h2(p[1]);
        c.g = h2(p[2]) * 16 + h2(p[3]);
        c.b = h2(p[4]) * 16 + h2(p[5]);
        if (std::strlen(p) >= 8) {
            c.a = h2(p[6]) * 16 + h2(p[7]);
        }
    }
    return c;
}

// Parse a string into an existing SettingValue, preserving its type.
void SettingsStore::ParseInto(SettingValue& target, const std::string& val) {
    std::visit([&](auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, bool>) {
            v = (val == "true" || val == "1");
        } else if constexpr (std::is_same_v<T, int>) {
            try { v = std::stoi(val); } catch (...) {}
        } else if constexpr (std::is_same_v<T, float>) {
            try { v = std::stof(val); } catch (...) {}
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (!val.empty() && val[0] == '"')
                v = val.substr(1, val.back() == '"' ? val.size() - 2 : val.size() - 1);
            else
                v = val;
        } else if constexpr (std::is_same_v<T, Color4>) {
            v = ParseColor(val);
        }
    }, target);
}

} // namespace ui
} // namespace koilo
