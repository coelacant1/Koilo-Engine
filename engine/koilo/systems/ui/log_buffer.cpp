// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file log_buffer.cpp
 * @brief Ring buffer log implementation.
 * @date 03/09/2026
 * @author Coela Can't
 */

#include "log_buffer.hpp"
#include <cctype>
#include <algorithm>

namespace koilo {

// =====================================================================
// Construction / Destruction
// =====================================================================

// Allocate ring buffer with given capacity
LogBuffer::LogBuffer(size_t capacity)
    : capacity_(capacity) {
    entries_ = new LogEntry[capacity_];
}

// Release heap-allocated entry array
LogBuffer::~LogBuffer() {
    delete[] entries_;
}

// =====================================================================
// Core operations
// =====================================================================

// Append an entry, advancing the ring head
void LogBuffer::Push(LogSeverity severity, const char* message, uint32_t timestamp) {
    LogEntry& entry = entries_[head_];
    entry.severity = severity;
    entry.timestamp = timestamp;
    entry.SetMessage(message);

    head_ = (head_ + 1) % capacity_;
    if (count_ < capacity_) ++count_;
    ++totalPushed_;
}

// Retrieve entry by logical index (0 = oldest)
const LogEntry* LogBuffer::At(size_t index) const {
    if (index >= count_) return nullptr;
    // Oldest entry is at (head_ - count_) mod capacity_
    size_t actual = (head_ + capacity_ - count_ + index) % capacity_;
    return &entries_[actual];
}

// Reset buffer state without deallocating
void LogBuffer::Clear() {
    head_ = 0;
    count_ = 0;
}

// =====================================================================
// Filtering / Search
// =====================================================================

// Count entries matching a severity level
size_t LogBuffer::CountBySeverity(LogSeverity severity) const {
    size_t n = 0;
    for (size_t i = 0; i < count_; ++i) {
        if (At(i)->severity == severity) ++n;
    }
    return n;
}

// Case-insensitive substring match helper
static bool ContainsCI(const char* haystack, const char* needle) {
    if (!needle[0]) return true;
    size_t nlen = std::strlen(needle);
    size_t hlen = std::strlen(haystack);
    if (nlen > hlen) return false;
    for (size_t i = 0; i <= hlen - nlen; ++i) {
        bool match = true;
        for (size_t j = 0; j < nlen; ++j) {
            if (std::tolower(static_cast<unsigned char>(haystack[i + j])) !=
                std::tolower(static_cast<unsigned char>(needle[j]))) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

// Find entries whose message contains a substring
size_t LogBuffer::Search(const char* substring, size_t* outIndices,
                         size_t maxResults) const {
    if (!substring || !outIndices || maxResults == 0) return 0;
    size_t found = 0;
    for (size_t i = 0; i < count_ && found < maxResults; ++i) {
        if (ContainsCI(At(i)->message, substring)) {
            outIndices[found++] = i;
        }
    }
    return found;
}

} // namespace koilo
