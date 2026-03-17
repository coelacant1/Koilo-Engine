// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file capabilities.hpp
 * @brief Capability bitmask for module permission checks.
 *
 * @date 11/03/2025
 * @author Coela
 */
#pragma once
#include <cstdint>

namespace koilo {

/// Capability bitmask for module permission checks.
/// Each module declares required caps; the kernel validates before granting access.
enum class Cap : uint32_t {
    None         = 0,
    MemoryAlloc  = 1 << 0,
    FileRead     = 1 << 1,
    FileWrite    = 1 << 2,
    GpuAccess    = 1 << 3,
    Network      = 1 << 4,
    ModuleLoad   = 1 << 5,
    ConsoleAdmin = 1 << 6,
    Debug        = 1 << 7,
    All          = 0xFFFFFFFF
};

inline constexpr Cap  operator|(Cap a, Cap b) { return static_cast<Cap>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
inline constexpr Cap  operator&(Cap a, Cap b) { return static_cast<Cap>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); }
inline Cap& operator|=(Cap& a, Cap b) { a = a | b; return a; }
inline Cap& operator&=(Cap& a, Cap b) { a = a & b; return a; }
inline constexpr Cap  operator~(Cap a) { return static_cast<Cap>(~static_cast<uint32_t>(a)); }

/// Check whether all required capabilities are present in the granted set.
inline constexpr bool HasCap(Cap granted, Cap required) {
    return (granted & required) == required;
}

#ifdef KL_BARE_METAL
    #define KL_CHECK_CAP(granted, required) ((void)0)
    #define KL_ALL_CAPS
#else
    /// Return false from the calling function if capability check fails.
    #define KL_CHECK_CAP(granted, required) \
        do { if (!::koilo::HasCap(granted, required)) return false; } while(0)
#endif

/// Default capabilities for trusted (built-in) modules.
constexpr Cap CAP_TRUSTED = Cap::All;

/// Default capabilities for untrusted (external) modules.
constexpr Cap CAP_SANDBOX = Cap::MemoryAlloc | Cap::FileRead | Cap::Debug;

/// Human-readable capability name (for console output).
inline const char* CapName(Cap c) {
    switch (c) {
        case Cap::MemoryAlloc:  return "memory.alloc";
        case Cap::FileRead:     return "file.read";
        case Cap::FileWrite:    return "file.write";
        case Cap::GpuAccess:    return "gpu.access";
        case Cap::Network:      return "network";
        case Cap::ModuleLoad:   return "module.load";
        case Cap::ConsoleAdmin: return "console.admin";
        case Cap::Debug:        return "debug";
        default:                return "unknown";
    }
}

/// List of individual cap bits for enumeration.
constexpr Cap ALL_CAPS[] = {
    Cap::MemoryAlloc, Cap::FileRead, Cap::FileWrite, Cap::GpuAccess,
    Cap::Network, Cap::ModuleLoad, Cap::ConsoleAdmin, Cap::Debug
};
constexpr int ALL_CAPS_COUNT = 8;

} // namespace koilo
