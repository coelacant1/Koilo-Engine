// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file unified_module.hpp
 * @brief Unified module interface with ordered lifecycle phases.
 *
 * @date 01/16/2026
 * @author Coela
 */
#pragma once

#include <cstdint>
#include <koilo/kernel/message_types.hpp>

namespace koilo {

class KoiloKernel;
class Color888;

// Module lifecycle phases for ordered initialization.
enum class ModulePhase : uint8_t {
    Core     = 0,  // Math, reflection, scene  always present
    System   = 1,  // Physics, audio, AI  independent subsystems
    Render   = 2,  // Effects, particles  render pipeline stages
    Overlay  = 3   // UI  rendered last, on top of everything
};

// Metadata for a loaded module.
struct ModuleInfo {
    const char* name    = nullptr;
    const char* version = nullptr;
    ModulePhase phase   = ModulePhase::System;
};

/// Base interface for all kernel-aware modules.
///
/// Modules implement IModule directly and receive a KoiloKernel&
/// in Initialize(). Use kernel services to access script engine,
/// time manager, etc.
class IModule {
public:
    virtual ~IModule() = default;

    /// Module identity (name, version, phase).
    virtual ModuleInfo GetInfo() const = 0;

    /// Called once during startup. Use kernel services, register globals.
    /// @return true on success, false to mark module as failed.
    virtual bool Initialize(KoiloKernel& kernel) = 0;

    /// Per-frame update. Called between BeginFrame/EndFrame.
    virtual void Update(float dt) = 0;

    /// Post-render pass. Default is no-op (most modules don't render).
    virtual void Render(Color888* buf, int w, int h) {
        (void)buf; (void)w; (void)h;
    }

    /// Handle a kernel message. Default is no-op.
    virtual void OnMessage(const Message& msg) { (void)msg; }

    /// Cleanup. Called during kernel shutdown in reverse init order.
    virtual void Shutdown() = 0;

protected:
    /// Kernel reference, set by the default Initialize() bridge or by subclasses.
    KoiloKernel* kernel_ = nullptr;
};

} // namespace koilo
