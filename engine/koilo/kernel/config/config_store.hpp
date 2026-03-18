// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file config_store.hpp
 * @brief Concrete IConfigProvider backed by an in-memory map with file I/O.
 *
 * Keys are dot-separated ("display.width", "render.vsync").
 * Values stored as variant<bool, int, float, string>.
 * File format is a simple line-oriented text:
 *
 *     # comment
 *     display.width = 1280
 *     render.vsync = true
 *     player.name = "Alice"
 *
 * @date 01/18/2026
 * @author Coela
 */
#pragma once

#include <koilo/kernel/config/config_provider.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <mutex>

namespace koilo {

class ConfigStore final : public IConfigProvider {
public:
    using Value = std::variant<bool, int, float, std::string>;

    ConfigStore() = default;

    // --- IConfigProvider ---

    bool        GetBool  (const char* key, bool        fallback = false) const override;
    int         GetInt   (const char* key, int         fallback = 0)     const override;
    float       GetFloat (const char* key, float       fallback = 0.0f)  const override;
    const char* GetString(const char* key, const char* fallback = "")    const override;

    void SetBool  (const char* key, bool value)        override;
    void SetInt   (const char* key, int value)         override;
    void SetFloat (const char* key, float value)       override;
    void SetString(const char* key, const char* value) override;

    bool Has   (const char* key) const override;
    void Remove(const char* key)       override;

    bool SaveToFile (const char* path) const override;
    bool LoadFromFile(const char* path)      override;

    // --- Extras (beyond interface) ---

    /// Return all keys, optionally filtered by a prefix ("display." etc.).
    std::vector<std::string> Keys(const char* prefix = "") const;

    /// Number of stored entries.
    size_t Count() const;

    /// True if any value changed since last MarkClean() / SaveToFile().
    bool IsDirty() const { return dirty_; }
    void MarkClean()     { dirty_ = false; }

    /// Synchronise Archive-flagged CVars into / out of this store.
    void PullArchiveCVars();
    void PushArchiveCVars() const;

private:
    mutable std::mutex              mu_;
    std::unordered_map<std::string, Value> entries_;
    mutable std::string             stringBuf_;  // temp for GetString return
    mutable bool                    dirty_ = false;
};

} // namespace koilo
