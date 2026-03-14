// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file localization.cpp
 * @brief Localization string table implementation.
 * @date 03/09/2026
 * @author Coela Can't
 */

#include "localization.hpp"

namespace koilo {

// =====================================================================
// Construction
// =====================================================================

// Initialize with default "en" locale
Localization::Localization() {
    locale_[0] = 'e'; locale_[1] = 'n'; locale_[2] = '\0';
}

// =====================================================================
// Locale management
// =====================================================================

// Switch the active locale identifier
void Localization::SetLocale(const char* locale) {
    if (!locale) return;
    size_t len = strlen(locale);
    if (len >= MAX_LOCALE_LEN) len = MAX_LOCALE_LEN - 1;
    memcpy(locale_, locale, len);
    locale_[len] = '\0';
}

// =====================================================================
// String table operations
// =====================================================================

// Add or update a key-value pair in the string table
bool Localization::Set(const char* key, const char* value) {
    if (!key || !value) return false;

    size_t keyLen = strlen(key);
    size_t valLen = strlen(value);
    if (keyLen >= MAX_KEY_LEN) keyLen = MAX_KEY_LEN - 1;
    if (valLen >= MAX_VALUE_LEN) valLen = MAX_VALUE_LEN - 1;

    // Check if key already exists - update value
    int existing = FindKey(key);
    if (existing >= 0) {
        // Can't shrink old value in-place; append new value
        if (bufUsed_ + valLen + 1 > MAX_STRING_BUF) return false;
        entries_[existing].valueOffset = static_cast<uint32_t>(bufUsed_);
        entries_[existing].valueLen = static_cast<uint16_t>(valLen);
        memcpy(stringBuf_ + bufUsed_, value, valLen);
        stringBuf_[bufUsed_ + valLen] = '\0';
        bufUsed_ += valLen + 1;
        return true;
    }

    // New entry
    if (count_ >= MAX_ENTRIES) return false;
    size_t needed = keyLen + 1 + valLen + 1;
    if (bufUsed_ + needed > MAX_STRING_BUF) return false;

    Entry& e = entries_[count_];
    e.keyOffset = static_cast<uint16_t>(bufUsed_);
    e.keyLen = static_cast<uint16_t>(keyLen);
    memcpy(stringBuf_ + bufUsed_, key, keyLen);
    stringBuf_[bufUsed_ + keyLen] = '\0';
    bufUsed_ += keyLen + 1;

    e.valueOffset = static_cast<uint32_t>(bufUsed_);
    e.valueLen = static_cast<uint16_t>(valLen);
    memcpy(stringBuf_ + bufUsed_, value, valLen);
    stringBuf_[bufUsed_ + valLen] = '\0';
    bufUsed_ += valLen + 1;

    ++count_;
    return true;
}

// Look up a localized string, falling back to the key itself
const char* Localization::Get(const char* key) const {
    if (!key) return "";
    int idx = FindKey(key);
    if (idx < 0) return key; // fallback: return the key itself
    return stringBuf_ + entries_[idx].valueOffset;
}

// Discard all entries and reset buffer usage
void Localization::Clear() {
    count_ = 0;
    bufUsed_ = 0;
}

// Linear scan for a key in the entry table
int Localization::FindKey(const char* key) const {
    size_t keyLen = strlen(key);
    for (size_t i = 0; i < count_; ++i) {
        if (entries_[i].keyLen == keyLen &&
            memcmp(stringBuf_ + entries_[i].keyOffset, key, keyLen) == 0) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace koilo
