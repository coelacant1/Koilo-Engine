// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file module.hpp
 * @brief Module descriptor and lifecycle state for the kernel module system.
 *
 * @date 11/18/2025
 * @author Coela
 */
#pragma once
#include <koilo/kernel/message_types.hpp>
#include <koilo/kernel/capabilities.hpp>
#include <cstdint>
#include "../registry/reflect_macros.hpp"

namespace koilo {

class KoiloKernel;

/// Module lifecycle state.
enum class ModuleState : uint8_t {
    Registered,
    Resolving,
    Initialized,
    Running,
    Faulted,       ///< Caught an exception; may be restarted.
    ShuttingDown,
    Unloaded,
    Error          ///< Permanent failure (init failed, too many faults).
};

/// Static module descriptor. Modules fill this out and register it with the kernel.
/// All function pointers are optional (nullptr = no-op for that lifecycle stage).
struct ModuleDesc {
    const char*  name         = nullptr;    // e.g. "koilo.audio", "koilo.console"
    uint32_t     version      = 0;          // KL_VERSION(major, minor, patch)
    Cap          requiredCaps = Cap::None;
    const char** dependencies = nullptr;    // null-terminated array of module names
    uint8_t      depCount     = 0;

    bool (*Init)(KoiloKernel& kernel);
    void (*Tick)(float dt);
    void (*OnMessage)(const Message& msg);
    void (*Shutdown)();

    KL_BEGIN_FIELDS(ModuleDesc)
        /* Function-pointer struct - no reflectable fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(ModuleDesc)
        /* Function-pointer struct - no reflectable methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ModuleDesc)
        /* No reflected ctors. */
    KL_END_DESCRIBE(ModuleDesc)

};

/// Pack a semantic version into a uint32.
constexpr uint32_t KL_VERSION(uint8_t major, uint8_t minor, uint16_t patch) {
    return (static_cast<uint32_t>(major) << 24) |
           (static_cast<uint32_t>(minor) << 16) |
           static_cast<uint32_t>(patch);
}

constexpr uint8_t  KL_VERSION_MAJOR(uint32_t v) { return static_cast<uint8_t>(v >> 24); }
constexpr uint8_t  KL_VERSION_MINOR(uint32_t v) { return static_cast<uint8_t>(v >> 16); }
constexpr uint16_t KL_VERSION_PATCH(uint32_t v) { return static_cast<uint16_t>(v); }

/// Registration macro for static modules (bare metal and built-in).
/// Usage: KL_REGISTER_MODULE(myModuleDesc)  at file scope.
#define KL_REGISTER_MODULE(desc) \
    namespace { \
        struct AutoRegister_##desc { \
            AutoRegister_##desc(); \
        } autoReg_##desc; \
    }
// Note: the constructor body must call kernel.RegisterModule(desc).
// Full auto-registration requires a global kernel pointer or registration list,
// which is set up in kernel.cpp.

} // namespace koilo
