// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file localization.hpp
 * @brief String table localization system.
 *
 * Loads key->localized-string tables. Supports multiple locales
 * with fallback. Provides L(key) lookup for scripts and C++.
 *
 * @date 03/09/2026
 * @author Coela Can't
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
    static constexpr size_t MAX_ENTRIES    = 4096;           ///< Maximum number of string entries.
    static constexpr size_t MAX_STRING_BUF = 256 * 1024;     ///< Total string storage (256KB).
    static constexpr size_t MAX_KEY_LEN    = 64;             ///< Maximum key length.
    static constexpr size_t MAX_VALUE_LEN  = 512;            ///< Maximum value length.
    static constexpr size_t MAX_LOCALE_LEN = 8;              ///< Maximum locale identifier length.

    Localization();
    ~Localization() = default;

    /** @brief Set the active locale (e.g., "en", "ja", "ar"). */
    void SetLocale(const char* locale);

    /** @brief Get the active locale. */
    const char* GetLocale() const { return locale_; }

    /** @brief True if the active locale uses right-to-left text direction. */
    bool IsRTL() const { return rtl_; }

    /** @brief Set RTL flag for the current locale. */
    void SetRTL(bool rtl) { rtl_ = rtl; }

    /**
     * @brief Add or update a string entry for the current locale.
     * @return True on success, false if table is full.
     */
    bool Set(const char* key, const char* value);

    /**
     * @brief Look up a localized string by key.
     * @return The value, or the key itself if not found (never null).
     */
    const char* Get(const char* key) const;

    /** @brief Convenience alias for Get(). */
    const char* L(const char* key) const { return Get(key); }

    /** @brief Number of entries in the current table. */
    size_t Count() const { return count_; }

    /** @brief Clear all entries. */
    void Clear();

    /** @brief Global font size multiplier for accessibility. */
    float FontScale() const { return fontScale_; }

    /** @brief Set the global font size multiplier (clamped to >= 0.1). */
    void SetFontScale(float scale) { fontScale_ = scale > 0.1f ? scale : 0.1f; }

private:
    /** @struct Entry @brief Index into the flat string buffer for one key-value pair. */
    struct Entry {
        uint16_t keyOffset;    ///< Byte offset of the key in stringBuf_.
        uint16_t keyLen;       ///< Length of the key string.
        uint32_t valueOffset;  ///< Byte offset of the value in stringBuf_.
        uint16_t valueLen;     ///< Length of the value string.

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

    Entry entries_[MAX_ENTRIES]{};  ///< Fixed-size array of string entries.
    size_t count_ = 0;              ///< Number of entries currently stored.

    char stringBuf_[MAX_STRING_BUF]{};  ///< Flat buffer holding all key and value strings.
    size_t bufUsed_ = 0;                ///< Bytes consumed in stringBuf_.

    char locale_[MAX_LOCALE_LEN]{"en"}; ///< Active locale identifier.
    bool rtl_ = false;                  ///< True if locale is right-to-left.
    float fontScale_ = 1.0f;            ///< Accessibility font scale multiplier.

    /** @brief Find entry index by key. Returns -1 if not found. */
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
