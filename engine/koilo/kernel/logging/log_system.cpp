// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file log_system.cpp
 * @brief LogSystem implementation - sinks, filtering, ring buffer, fprintf fallback.
 *
 * @date 11/20/2025
 * @author Coela Can't
 */

#include "log_system.hpp"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <chrono>
#include <algorithm>

namespace koilo {

// ---------------------------------------------------------------------------
// Global log system pointer + free functions
// ---------------------------------------------------------------------------

static LogSystem* g_logSystem = nullptr;

LogSystem* GetLogSystem() { return g_logSystem; }

void LogEmit(LogLevel level, const char* channel, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogEmitV(level, channel, fmt, args);
    va_end(args);
}

void LogEmitV(LogLevel level, const char* channel, const char* fmt, va_list args) {
    if (g_logSystem) {
        g_logSystem->Emit(level, channel, fmt, args);
        return;
    }
    // Fallback: direct fprintf
    FILE* out = (level <= LogLevel::Warn) ? stderr : stdout;
    if (level <= LogLevel::Warn) {
        fprintf(out, "[%s %s] ", channel, LogLevelStr(level));
    } else {
        fprintf(out, "[%s] ", channel);
    }
    vfprintf(out, fmt, args);
    fprintf(out, "\n");
}

// ---------------------------------------------------------------------------
// LogSystem lifetime
// ---------------------------------------------------------------------------

LogSystem::LogSystem() {
    history_.resize(kHistoryCapacity);
    auto now = std::chrono::steady_clock::now();
    startMs_ = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count());

    // Default sink: colorized console output
    AddSink("console", [](const LogRecord& rec) {
        FILE* out = (rec.level <= LogLevel::Warn) ? stderr : stdout;
        // ANSI colors: red=error, yellow=warn, default=info, dim=debug/trace
        const char* colorOn  = "";
        const char* colorOff = "";
        switch (rec.level) {
            case LogLevel::Error: colorOn = "\033[31m"; colorOff = "\033[0m"; break;
            case LogLevel::Warn:  colorOn = "\033[33m"; colorOff = "\033[0m"; break;
            case LogLevel::Debug: colorOn = "\033[2m";  colorOff = "\033[0m"; break;
            case LogLevel::Trace: colorOn = "\033[2m";  colorOff = "\033[0m"; break;
            default: break;
        }
        if (rec.level <= LogLevel::Warn) {
            fprintf(out, "%s[%s %s] %s%s\n", colorOn, rec.channel,
                    LogLevelStr(rec.level), rec.message, colorOff);
        } else {
            fprintf(out, "%s[%s] %s%s\n", colorOn, rec.channel,
                    rec.message, colorOff);
        }
        fflush(out);
    });

    g_logSystem = this;
}

LogSystem::~LogSystem() {
    if (g_logSystem == this) g_logSystem = nullptr;
    CloseLogFile();
}

// ---------------------------------------------------------------------------
// Channel filtering
// ---------------------------------------------------------------------------

void LogSystem::SetChannelLevel(const std::string& channel, LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    channelLevels_[channel] = level;
}

void LogSystem::ClearChannelLevel(const std::string& channel) {
    std::lock_guard<std::mutex> lock(mutex_);
    channelLevels_.erase(channel);
}

void LogSystem::MuteChannel(const std::string& channel) {
    std::lock_guard<std::mutex> lock(mutex_);
    mutedChannels_.insert(channel);
}

void LogSystem::UnmuteChannel(const std::string& channel) {
    std::lock_guard<std::mutex> lock(mutex_);
    mutedChannels_.erase(channel);
}

bool LogSystem::IsChannelMuted(const std::string& channel) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mutedChannels_.count(channel) > 0;
}

bool LogSystem::ShouldLog(LogLevel level, const char* channel) const {
    // Check muted
    if (mutedChannels_.count(channel)) return false;

    // Check per-channel override
    auto it = channelLevels_.find(channel);
    LogLevel threshold = (it != channelLevels_.end()) ? it->second : globalLevel_;
    return level <= threshold;
}

// ---------------------------------------------------------------------------
// String interning
// ---------------------------------------------------------------------------

const char* LogSystem::InternChannel(const char* channel) {
    auto it = internedChannels_.find(channel);
    if (it != internedChannels_.end()) return it->second->c_str();
    auto owned = std::make_unique<std::string>(channel);
    const char* ptr = owned->c_str();
    internedChannels_[channel] = std::move(owned);
    return ptr;
}

// ---------------------------------------------------------------------------
// Emit
// ---------------------------------------------------------------------------

void LogSystem::Emit(LogLevel level, const char* channel, const char* fmt, va_list args) {
    std::lock_guard<std::mutex> lock(mutex_);

    knownChannels_.insert(channel);

    if (!ShouldLog(level, channel)) return;

    // Build record
    LogRecord rec;
    rec.level = level;
    rec.channel = InternChannel(channel);
    vsnprintf(rec.message, sizeof(rec.message), fmt, args);
    auto now = std::chrono::steady_clock::now();
    rec.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count()) - startMs_;

    // Store in ring buffer
    history_[historyHead_] = rec;
    historyHead_ = (historyHead_ + 1) % kHistoryCapacity;
    if (historyCount_ < kHistoryCapacity) ++historyCount_;

    // Dispatch to sinks
    for (auto& entry : sinks_) {
        entry.sink(rec);
    }

    // File sink
    if (logFile_) {
        double secs = static_cast<double>(rec.timestamp) / 1000.0;
        fprintf(logFile_, "[%8.3f] [%-5s] [%s] %s\n",
                secs, LogLevelStr(level), rec.channel, rec.message);
        fflush(logFile_);
    }
}

// ---------------------------------------------------------------------------
// Sinks
// ---------------------------------------------------------------------------

size_t LogSystem::AddSink(const std::string& name, LogSink sink) {
    // No lock needed - only called from constructor or under lock
    size_t id = nextSinkId_++;
    sinks_.push_back({id, name, std::move(sink)});
    return id;
}

void LogSystem::RemoveSink(size_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.erase(
        std::remove_if(sinks_.begin(), sinks_.end(),
                        [id](const SinkEntry& e) { return e.id == id; }),
        sinks_.end());
}

// ---------------------------------------------------------------------------
// History
// ---------------------------------------------------------------------------

size_t LogSystem::HistoryCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return historyCount_;
}

const LogRecord* LogSystem::HistoryAt(size_t index) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (index >= historyCount_) return nullptr;
    size_t start = (historyHead_ + kHistoryCapacity - historyCount_) % kHistoryCapacity;
    size_t pos = (start + index) % kHistoryCapacity;
    return &history_[pos];
}

void LogSystem::ClearHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    historyHead_ = 0;
    historyCount_ = 0;
}

std::vector<std::string> LogSystem::ListChannels() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result(knownChannels_.begin(), knownChannels_.end());
    std::sort(result.begin(), result.end());
    return result;
}

// ---------------------------------------------------------------------------
// File logging
// ---------------------------------------------------------------------------

void LogSystem::OpenLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    CloseLogFile();
    logFile_ = fopen(path.c_str(), "w");
    if (logFile_) {
        fprintf(logFile_, "=== Koilo Engine Log ===\n");
        fflush(logFile_);
    }
}

void LogSystem::CloseLogFile() {
    if (logFile_) {
        fclose(logFile_);
        logFile_ = nullptr;
    }
}

} // namespace koilo
