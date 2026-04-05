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
#include <koilo/kernel/config/config_store.hpp>
#include <koilo/kernel/thread_pool.hpp>
#include <koilo/kernel/engine_error.hpp>
#include <koilo/kernel/task_group.hpp>
#include <koilo/kernel/schema_migrator.hpp>
#include <koilo/core/time/timemanager.hpp>
#include <koilo/debug/debugdraw.hpp>
#include <koilo/debug/profiler.hpp>
#include "../registry/reflect_macros.hpp"
#include <memory>

namespace koilo { class Sky; }

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
    ConfigStore&      GetConfig()   { return config_; }
    ThreadPool&       Pool()        { return pool_; }
    ErrorHistory&     Errors()      { return errors_; }
    SchemaMigrator&   Migrator()    { return migrator_; }

    /// Report a structured engine error. Logs, dispatches to MessageBus, stores in history.
    void ReportError(const EngineError& err);

    /// Convenience: build and report an error with printf-style message.
    template<typename... Args>
    void ReportError(ErrorSeverity sev, ErrorSystem sys,
                     const char* fmt, Args&&... args) {
        char buf[256];
        std::snprintf(buf, sizeof(buf), fmt, std::forward<Args>(args)...);
        ReportError(EngineError::Make(sev, sys, nullptr, buf));
    }

    /// Execute a scoped task group. All tasks spawned within the lambda are
    /// guaranteed to complete before TaskScope returns.
    template<typename F>
    void TaskScope(const std::string& name, F&& body) {
        TaskGroup group(name, pool_);
        body(group);
        // ~TaskGroup waits for all children
    }

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

    /// Check whether a module has a capability, logging a warning on failure.
    /// Returns true if the module holds the required cap.
    bool RequireCap(ModuleId caller, Cap required, const char* operation = nullptr);

    bool IsRunning() const { return running_; }

private:
    ArenaAllocator  frameArena_;
    LinearAllocator scratch_;
    MessageBus      bus_;
    ServiceRegistry services_;
    ModuleManager   modules_;
    ConfigStore     config_;
    ThreadPool      pool_;
    ErrorHistory    errors_;
    SchemaMigrator  migrator_;
    bool            running_ = false;

    // Kernel-owned singletons (installed via SetInstance on construction)
    TimeManager             timeManager_;
    DebugDraw               debugDraw_;
    Profiler                profiler_;
    std::unique_ptr<Sky>    sky_;

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
