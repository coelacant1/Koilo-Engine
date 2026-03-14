// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file log_buffer.hpp
 * @brief Ring buffer of log entries with severity levels and filtering.
 *
 * Designed for the editor console panel. Stores the last N entries
 * in a fixed-size ring buffer with zero heap allocation in steady state.
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

/** @enum LogSeverity @brief Severity level for log entries. */
enum class LogSeverity : uint8_t {
    Info = 0,  ///< Informational message.
    Warning,   ///< Warning message.
    Error,     ///< Error message.
    Debug      ///< Debug-only message.
};

/** @struct LogEntry @brief A single log entry with severity, timestamp, and message. */
struct LogEntry {
    static constexpr size_t MAX_MSG_LEN = 256; ///< Maximum message length.

    LogSeverity severity = LogSeverity::Info;   ///< Severity level.
    uint32_t    timestamp = 0;                  ///< Frame number or milliseconds.
    char        message[MAX_MSG_LEN]{};         ///< Null-terminated message text.

    /** @brief Set the message text, truncating if necessary. */
    void SetMessage(const char* msg) {
        if (!msg) { message[0] = '\0'; return; }
        size_t len = std::strlen(msg);
        if (len >= MAX_MSG_LEN) len = MAX_MSG_LEN - 1;
        std::memcpy(message, msg, len);
        message[len] = '\0';
    }

    KL_BEGIN_FIELDS(LogEntry)
        KL_FIELD(LogEntry, severity, "Severity", 0, 3),
        KL_FIELD(LogEntry, timestamp, "Timestamp", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(LogEntry)
        KL_METHOD_AUTO(LogEntry, SetMessage, "Set message")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(LogEntry)
        /* No reflected ctors. */
    KL_END_DESCRIBE(LogEntry)

};

/**
 * @class LogBuffer
 * @brief Fixed-size ring buffer of log entries.
 */
class LogBuffer {
public:
    static constexpr size_t DEFAULT_CAPACITY = 1024;

    explicit LogBuffer(size_t capacity = DEFAULT_CAPACITY);
    ~LogBuffer();

    /** @brief Add a log entry. */
    void Push(LogSeverity severity, const char* message, uint32_t timestamp = 0);

    /** @brief Number of entries currently stored. */
    size_t Count() const { return count_; }

    /** @brief Get entry by index (0 = oldest visible entry). */
    const LogEntry* At(size_t index) const;

    /** @brief Clear all entries. */
    void Clear();

    /** @brief Total number of entries ever pushed (including those evicted). */
    size_t TotalPushed() const { return totalPushed_; }

    /** @brief Capacity of the ring buffer. */
    size_t Capacity() const { return capacity_; }

    // -- Filtering ---------------------------------------------------

    /** @brief Count entries matching severity filter. */
    size_t CountBySeverity(LogSeverity severity) const;

    /**
     * @brief Search entries containing substring (case-insensitive).
     *
     * Returns indices into the ring buffer (call At() with them).
     * Writes up to maxResults indices. Returns number written.
     */
    size_t Search(const char* substring, size_t* outIndices,
                  size_t maxResults) const;

private:
    LogEntry* entries_ = nullptr;  ///< Heap-allocated ring buffer.
    size_t capacity_ = 0;          ///< Maximum number of entries.
    size_t head_ = 0;              ///< Next write position.
    size_t count_ = 0;             ///< Current number of valid entries.
    size_t totalPushed_ = 0;       ///< Lifetime push counter.

    KL_BEGIN_FIELDS(LogBuffer)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(LogBuffer)
        KL_METHOD_AUTO(LogBuffer, Capacity, "Capacity"),
        KL_METHOD_AUTO(LogBuffer, CountBySeverity, "Count by severity")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(LogBuffer)
        KL_CTOR0(LogBuffer)
    KL_END_DESCRIBE(LogBuffer)

};

} // namespace koilo
