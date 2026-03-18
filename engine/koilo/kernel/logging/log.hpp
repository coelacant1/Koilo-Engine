// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
/**
 * @file log.hpp
 * @brief Structured logging for KoiloEngine.
 *
 * Provides KL_LOG / KL_WARN / KL_ERR / KL_DBG macros that route through
 * a central LogSystem with per-channel filtering, multiple sinks, and
 * ring-buffer history.  Falls back to fprintf when no LogSystem exists
 * (standalone tests, early startup).
 *
 * Usage:
 *   KL_LOG("VulkanBackend", "Swapchain: %ux%u", w, h);
 *   KL_WARN("Kernel", "Module '%s' already registered", name);
 *   KL_ERR("KSL", "Compile failed: %s", err.c_str());
 *   KL_DBG("Profiler", "Frame %u: %.1fms", frame, ms);
 *   KL_TRACE("MessageBus", "Dispatch type=%u", type);
 *
 * @date 11/14/2025
 * @author Coela Can't
 */

#include <cstdio>
#include <cstdarg>
#include <cstdint>

namespace koilo {

/// Log severity levels (ordered by decreasing severity).
enum class LogLevel : uint8_t {
    Error = 0,
    Warn  = 1,
    Info  = 2,
    Debug = 3,
    Trace = 4
};

/// Convert LogLevel to short string.
inline const char* LogLevelStr(LogLevel lv) {
    switch (lv) {
        case LogLevel::Error: return "ERROR";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Debug: return "DBG";
        case LogLevel::Trace: return "TRACE";
    }
    return "?";
}

/// Forward-declared LogSystem (implemented in log_system.cpp).
class LogSystem;

/// Global pointer set by LogSystem on construction.
/// When null, macros fall back to fprintf.
LogSystem* GetLogSystem();

/// Core emit function - called by macros.
void LogEmit(LogLevel level, const char* channel, const char* fmt, ...)
    __attribute__((format(printf, 3, 4)));

/// Variant for va_list forwarding.
void LogEmitV(LogLevel level, const char* channel, const char* fmt, va_list args);

} // namespace koilo

// ---------------------------------------------------------------------------
// Convenience macros
// ---------------------------------------------------------------------------
// Each macro passes __FILE__ and __LINE__ implicitly through the channel tag.
// Channel is a short human-readable string like "VulkanBackend" or "Kernel".

#define KL_LOG(channel, fmt, ...)   ::koilo::LogEmit(::koilo::LogLevel::Info,  channel, fmt, ##__VA_ARGS__)
#define KL_WARN(channel, fmt, ...)  ::koilo::LogEmit(::koilo::LogLevel::Warn,  channel, fmt, ##__VA_ARGS__)
#define KL_ERR(channel, fmt, ...)   ::koilo::LogEmit(::koilo::LogLevel::Error, channel, fmt, ##__VA_ARGS__)
#define KL_DBG(channel, fmt, ...)   ::koilo::LogEmit(::koilo::LogLevel::Debug, channel, fmt, ##__VA_ARGS__)
#define KL_TRACE(channel, fmt, ...) ::koilo::LogEmit(::koilo::LogLevel::Trace, channel, fmt, ##__VA_ARGS__)
