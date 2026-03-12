// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file localization.hpp
 * @brief String table localization system.
 *
 * Loads key->localized-string tables. Supports multiple locales
 * with fallback. Provides L(key) lookup for scripts and C++.
 *
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

/**
 * @class Localization
 * @brief Key-value string table with locale switching.
 *
 * Stores strings in a flat pre-allocated buffer. Keys and values
 * are stored contiguously. Lookup is linear scan (sufficient for
 * typical game string counts < 5000).
 */
class Localization {
public:
    static constexpr size_t MAX_ENTRIES    = 4096;
    static constexpr size_t MAX_STRING_BUF = 256 * 1024; // 256KB
    static constexpr size_t MAX_KEY_LEN    = 64;
    static constexpr size_t MAX_VALUE_LEN  = 512;
    static constexpr size_t MAX_LOCALE_LEN = 8;

    Localization();
    ~Localization() = default;

    /// Set the active locale (e.g., "en", "ja", "ar").
    void SetLocale(const char* locale);

    /// Get the active locale.
    const char* GetLocale() const { return locale_; }

    /// True if the active locale uses right-to-left text direction.
    bool IsRTL() const { return rtl_; }

    /// Set RTL flag for the current locale.
    void SetRTL(bool rtl) { rtl_ = rtl; }

    /// Add or update a string entry for the current locale.
    /// Returns true on success, false if table is full.
    bool Set(const char* key, const char* value);

    /// Look up a localized string by key.
    /// Returns the value, or the key itself if not found (never null).
    const char* Get(const char* key) const;

    /// Convenience alias for Get().
    const char* L(const char* key) const { return Get(key); }

    /// Number of entries in the current table.
    size_t Count() const { return count_; }

    /// Clear all entries.
    void Clear();

    /// Global font size multiplier for accessibility.
    float FontScale() const { return fontScale_; }
    void SetFontScale(float scale) { fontScale_ = scale > 0.1f ? scale : 0.1f; }

private:
    struct Entry {
        uint16_t keyOffset;
        uint16_t keyLen;
        uint32_t valueOffset;
        uint16_t valueLen;

        KL_BEGIN_FIELDS(Entry)
            KL_FIELD(Entry, keyOffset, "Key offset", 0, 65535),
            KL_FIELD(Entry, keyLen, "Key len", 0, 65535),
            KL_FIELD(Entry, valueOffset, "Value offset", 0, 4294967295),
            KL_FIELD(Entry, valueLen, "Value len", 0, 65535)
        KL_END_FIELDS

        KL_BEGIN_METHODS(Entry)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(Entry)
            /* No reflected ctors. */
        KL_END_DESCRIBE(Entry)

    };

    Entry entries_[MAX_ENTRIES]{};
    size_t count_ = 0;

    char stringBuf_[MAX_STRING_BUF]{};
    size_t bufUsed_ = 0;

    char locale_[MAX_LOCALE_LEN]{"en"};
    bool rtl_ = false;
    float fontScale_ = 1.0f;

    /// Find entry index by key. Returns -1 if not found.
    int FindKey(const char* key) const;

    KL_BEGIN_FIELDS(Localization)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Localization)
        KL_METHOD_AUTO(Localization, IsRTL, "Is rtl"),
        KL_METHOD_AUTO(Localization, SetRTL, "Set rtl"),
        KL_METHOD_AUTO(Localization, Set, "Set"),
        KL_METHOD_AUTO(Localization, Count, "Count"),
        KL_METHOD_AUTO(Localization, Clear, "Clear"),
        KL_METHOD_AUTO(Localization, SetFontScale, "Set font scale")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Localization)
        KL_CTOR0(Localization)
    KL_END_DESCRIBE(Localization)

};

} // namespace koilo
