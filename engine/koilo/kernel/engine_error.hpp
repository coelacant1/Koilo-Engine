// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file engine_error.hpp
 * @brief Structured error taxonomy with categorized error codes.
 *
 * Provides EngineError - a structured error object with severity, system,
 * error code, message, and optional recovery suggestion. Replaces ad-hoc
 * string-based error reporting throughout the engine.
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>

namespace koilo {

/// Severity classification for engine errors.
enum class ErrorSeverity : uint8_t {
    Diagnostic,   ///< Informational - something unusual but handled
    Recoverable,  ///< Error occurred but fallback path was taken
    Degraded,     ///< Subsystem running in reduced capacity
    Fatal         ///< Unrecoverable - subsystem or engine must shut down
};

/// Subsystem that originated the error.
enum class ErrorSystem : uint8_t {
    Kernel,
    Render,
    Asset,
    Script,
    Audio,
    Physics,
    Input,
    UI,
    ECS,
    Network,
    Module,
    Other
};

/// A structured engine error with severity, origin, and documentation-linkable code.
struct EngineError {
    ErrorSeverity severity = ErrorSeverity::Recoverable;
    ErrorSystem   system   = ErrorSystem::Kernel;
    char          code[48] = {};     ///< Unique error code (e.g., "MISSING_TEXTURE")
    char          message[256] = {}; ///< Human-readable description
    char          suggestion[128] = {}; ///< Recovery hint (optional)
    uint64_t      timestamp = 0;     ///< Monotonic ms (filled by ErrorHistory)

    /// Convenience constructor.
    static EngineError Make(ErrorSeverity sev, ErrorSystem sys,
                            const char* code, const char* msg,
                            const char* suggest = nullptr) {
        EngineError e{};
        e.severity = sev;
        e.system = sys;
        if (code) std::strncpy(e.code, code, sizeof(e.code) - 1);
        if (msg) std::strncpy(e.message, msg, sizeof(e.message) - 1);
        if (suggest) std::strncpy(e.suggestion, suggest, sizeof(e.suggestion) - 1);
        return e;
    }
};

inline const char* ErrorSeverityName(ErrorSeverity s) {
    switch (s) {
        case ErrorSeverity::Diagnostic:  return "DIAG";
        case ErrorSeverity::Recoverable: return "RECV";
        case ErrorSeverity::Degraded:    return "DEGD";
        case ErrorSeverity::Fatal:       return "FATAL";
        default:                         return "?";
    }
}

inline const char* ErrorSystemName(ErrorSystem s) {
    switch (s) {
        case ErrorSystem::Kernel:  return "kernel";
        case ErrorSystem::Render:  return "render";
        case ErrorSystem::Asset:   return "asset";
        case ErrorSystem::Script:  return "script";
        case ErrorSystem::Audio:   return "audio";
        case ErrorSystem::Physics: return "physics";
        case ErrorSystem::Input:   return "input";
        case ErrorSystem::UI:      return "ui";
        case ErrorSystem::ECS:     return "ecs";
        case ErrorSystem::Network: return "network";
        case ErrorSystem::Module:  return "module";
        case ErrorSystem::Other:   return "other";
        default:                   return "?";
    }
}

/// Parse a system name string to ErrorSystem enum. Returns Other on no match.
inline ErrorSystem ParseErrorSystem(const char* name) {
    if (!name) return ErrorSystem::Other;
    if (std::strcmp(name, "kernel")  == 0) return ErrorSystem::Kernel;
    if (std::strcmp(name, "render")  == 0) return ErrorSystem::Render;
    if (std::strcmp(name, "asset")   == 0) return ErrorSystem::Asset;
    if (std::strcmp(name, "script")  == 0) return ErrorSystem::Script;
    if (std::strcmp(name, "audio")   == 0) return ErrorSystem::Audio;
    if (std::strcmp(name, "physics") == 0) return ErrorSystem::Physics;
    if (std::strcmp(name, "input")   == 0) return ErrorSystem::Input;
    if (std::strcmp(name, "ui")      == 0) return ErrorSystem::UI;
    if (std::strcmp(name, "ecs")     == 0) return ErrorSystem::ECS;
    if (std::strcmp(name, "network") == 0) return ErrorSystem::Network;
    if (std::strcmp(name, "module")  == 0) return ErrorSystem::Module;
    return ErrorSystem::Other;
}

/// Parse severity name to enum. Returns Recoverable on no match.
inline ErrorSeverity ParseErrorSeverity(const char* name) {
    if (!name) return ErrorSeverity::Recoverable;
    if (std::strcmp(name, "fatal") == 0)  return ErrorSeverity::Fatal;
    if (std::strcmp(name, "degraded") == 0) return ErrorSeverity::Degraded;
    if (std::strcmp(name, "recoverable") == 0) return ErrorSeverity::Recoverable;
    if (std::strcmp(name, "diagnostic") == 0 || std::strcmp(name, "diag") == 0)
        return ErrorSeverity::Diagnostic;
    return ErrorSeverity::Recoverable;
}

// ---- Error History (ring buffer) ----

/// Fixed-size ring buffer of recent engine errors.
class ErrorHistory {
public:
    static constexpr size_t kDefaultCapacity = 256;

    explicit ErrorHistory(size_t capacity = kDefaultCapacity)
        : capacity_(capacity) {
        buffer_.resize(capacity);
    }

    /// Record an error into the ring buffer. Fills timestamp automatically.
    void Push(const EngineError& err) {
        EngineError e = err;
        e.timestamp = elapsedMs_++;
        buffer_[head_] = e;
        head_ = (head_ + 1) % capacity_;
        if (count_ < capacity_) ++count_;
    }

    /// Number of errors currently stored.
    size_t Count() const { return count_; }

    /// Access by index (0 = oldest). Returns nullptr if out of range.
    const EngineError* At(size_t index) const {
        if (index >= count_) return nullptr;
        size_t start = (head_ + capacity_ - count_) % capacity_;
        size_t pos = (start + index) % capacity_;
        return &buffer_[pos];
    }

    /// Clear all stored errors.
    void Clear() {
        head_ = 0;
        count_ = 0;
    }

    /// Get all errors matching optional filters.
    std::vector<const EngineError*> Query(
        const ErrorSeverity* severity = nullptr,
        const ErrorSystem* system = nullptr) const
    {
        std::vector<const EngineError*> results;
        for (size_t i = 0; i < count_; ++i) {
            const EngineError* e = At(i);
            if (!e) continue;
            if (severity && e->severity != *severity) continue;
            if (system && e->system != *system) continue;
            results.push_back(e);
        }
        return results;
    }

    /// Set the monotonic timestamp counter (for testing or sync with LogSystem).
    void SetElapsedMs(uint64_t ms) { elapsedMs_ = ms; }

private:
    std::vector<EngineError> buffer_;
    size_t capacity_;
    size_t head_  = 0;
    size_t count_ = 0;
    uint64_t elapsedMs_ = 0;
};

} // namespace koilo
