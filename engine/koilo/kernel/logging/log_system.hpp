// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
/**
 * @file log_system.hpp
 * @brief LogSystem - central log router with sinks, channel filtering, and history.
 *
 * @date 11/20/2025
 * @author Coela Can't
 */

#include "log.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace koilo {

/// A single formatted log entry stored in the ring buffer.
struct LogRecord {
    LogLevel    level;
    const char* channel;     // Points to interned string (stable pointer)
    char        message[512];
    uint64_t    timestamp;   // Monotonic ms since LogSystem creation
};

/// Sink callback signature: receives the fully formatted record.
using LogSink = std::function<void(const LogRecord& record)>;

class LogSystem {
public:
    LogSystem();
    ~LogSystem();

    // Non-copyable
    LogSystem(const LogSystem&) = delete;
    LogSystem& operator=(const LogSystem&) = delete;

    /// Set the global minimum log level (messages below this are discarded).
    void SetLevel(LogLevel level) { globalLevel_ = level; }
    LogLevel GetLevel() const { return globalLevel_; }

    /// Per-channel level override. Channel matching is exact (case-sensitive).
    void SetChannelLevel(const std::string& channel, LogLevel level);
    void ClearChannelLevel(const std::string& channel);

    /// Mute/unmute a channel entirely.
    void MuteChannel(const std::string& channel);
    void UnmuteChannel(const std::string& channel);
    bool IsChannelMuted(const std::string& channel) const;

    /// Add a named sink. Returns sink id for removal.
    size_t AddSink(const std::string& name, LogSink sink);
    void RemoveSink(size_t id);

    /// Core emit - formats message and dispatches to sinks.
    void Emit(LogLevel level, const char* channel, const char* fmt, va_list args);

    // --- History ring buffer ---
    size_t HistoryCount() const;
    const LogRecord* HistoryAt(size_t index) const; // 0 = oldest
    void ClearHistory();

    /// List all channels that have emitted at least one message.
    std::vector<std::string> ListChannels() const;

    /// Enable/disable file logging.
    void OpenLogFile(const std::string& path);
    void CloseLogFile();

private:
    LogLevel globalLevel_ = LogLevel::Info;

    struct SinkEntry {
        size_t      id;
        std::string name;
        LogSink     sink;
    };
    std::vector<SinkEntry> sinks_;
    size_t nextSinkId_ = 1;

    // Channel filtering
    std::unordered_map<std::string, LogLevel> channelLevels_;
    std::unordered_set<std::string> mutedChannels_;

    // Channel tracking
    mutable std::unordered_set<std::string> knownChannels_;

    // History ring buffer
    static constexpr size_t kHistoryCapacity = 2048;
    std::vector<LogRecord> history_;
    size_t historyHead_  = 0;
    size_t historyCount_ = 0;

    // File sink
    FILE* logFile_ = nullptr;

    // Monotonic start time
    uint64_t startMs_;

    mutable std::mutex mutex_;

    bool ShouldLog(LogLevel level, const char* channel) const;
    const char* InternChannel(const char* channel);

    // Interned channel strings (stable pointers for LogRecord)
    std::unordered_map<std::string, std::unique_ptr<std::string>> internedChannels_;
};

} // namespace koilo
