// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kernel.hpp
 * @brief Central kernel object owning all kernel subsystems.
 *
 * @date 11/22/2025
 * @author Coela
 */
#pragma once
#include <koilo/kernel/memory/arena_allocator.hpp>
#include <koilo/kernel/memory/linear_allocator.hpp>
#include <koilo/kernel/message_bus.hpp>
#include <koilo/kernel/service_registry.hpp>
#include <koilo/kernel/module_manager.hpp>
#include <koilo/kernel/capabilities.hpp>
#include "../registry/reflect_macros.hpp"

namespace koilo {

/// Central kernel object. Owns all kernel subsystems.
/// Typically one instance per application, created at startup.
class KoiloKernel {
public:
    struct Config {
        size_t frameArenaSize   = 4 * 1024 * 1024;  // 4 MB default
        size_t scratchSize      = 1 * 1024 * 1024;   // 1 MB default
        size_t messageBusSize   = 1024;               // ring buffer capacity

        KL_BEGIN_FIELDS(Config)
            KL_FIELD(Config, frameArenaSize, "Frame arena size", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(Config)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(Config)
            /* No reflected ctors. */
        KL_END_DESCRIBE(Config)

    };

    KoiloKernel();
    explicit KoiloKernel(const Config& config);
    ~KoiloKernel();

    KoiloKernel(const KoiloKernel&) = delete;
    KoiloKernel& operator=(const KoiloKernel&) = delete;

    // --- Subsystem access ---

    ArenaAllocator&   FrameArena()  { return frameArena_; }
    LinearAllocator&  Scratch()     { return scratch_; }
    MessageBus&       Messages()    { return bus_; }
    ServiceRegistry&  Services()    { return services_; }
    ModuleManager&    Modules()     { return modules_; }

    // --- Module convenience ---

    /// Register a module with the kernel.
    ModuleId RegisterModule(const ModuleDesc& desc, Cap caps = CAP_TRUSTED);

    /// Resolve dependencies and initialize all modules.
    bool InitializeModules();

    // --- Frame lifecycle ---

    /// Call at the start of each frame. Dispatches queued messages.
    void BeginFrame();

    /// Call at the end of each frame. Resets frame arena.
    void EndFrame();

    /// Tick all modules. Call between BeginFrame/EndFrame.
    void Tick(float dt);

    /// Shut down all modules and clean up.
    void Shutdown();

    bool IsRunning() const { return running_; }

private:
    ArenaAllocator  frameArena_;
    LinearAllocator scratch_;
    MessageBus      bus_;
    ServiceRegistry services_;
    ModuleManager   modules_;
    bool            running_ = false;

    KL_BEGIN_FIELDS(KoiloKernel)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(KoiloKernel)
        KL_METHOD_AUTO(KoiloKernel, Scratch, "Scratch"),
        KL_METHOD_AUTO(KoiloKernel, Messages, "Messages"),
        KL_METHOD_AUTO(KoiloKernel, Services, "Services"),
        KL_METHOD_AUTO(KoiloKernel, Modules, "Modules"),
        KL_METHOD_AUTO(KoiloKernel, RegisterModule, "Register module"),
        KL_METHOD_AUTO(KoiloKernel, IsRunning, "Is running")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KoiloKernel)
        KL_CTOR0(KoiloKernel)
    KL_END_DESCRIBE(KoiloKernel)

};

} // namespace koilo
