#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <koilo/systems/render/sky/sky.hpp>

namespace koilo {

KoiloKernel::KoiloKernel()
    : KoiloKernel(Config{}) {
}

KoiloKernel::KoiloKernel(const Config& config)
    : frameArena_(config.frameArenaSize)
    , scratch_(config.scratchSize)
    , bus_(config.messageBusSize)
    , modules_(bus_, services_)
    , sky_(std::make_unique<Sky>()) {
    services_.SetBus(&bus_);

    // Install kernel-owned instances as the active singletons
    TimeManager::SetInstance(&timeManager_);
    DebugDraw::SetInstance(&debugDraw_);
    Profiler::SetInstance(&profiler_);
    Sky::SetInstance(sky_.get());

    // Register config as a kernel service
    services_.Register("config", &config_);
    services_.Register("thread_pool", &pool_);
}

KoiloKernel::~KoiloKernel() {
    if (running_) {
        Shutdown();
    }
    // Clear singleton redirects before members are destroyed
    TimeManager::ClearInstance();
    DebugDraw::ClearInstance();
    Profiler::ClearInstance();
    Sky::ClearInstance();
}

ModuleId KoiloKernel::RegisterModule(const ModuleDesc& desc, Cap caps) {
    return modules_.RegisterModule(desc, caps);
}

bool KoiloKernel::InitializeModules() {
    if (!modules_.InitializeAll(*this)) {
        KL_ERR("Kernel", "Module initialization failed");
        return false;
    }
    running_ = true;
    return true;
}

void KoiloKernel::BeginFrame() {
    bus_.Send(MakeSignal(MSG_FRAME_BEGIN));
    bus_.Dispatch();
}

void KoiloKernel::EndFrame() {
    bus_.Send(MakeSignal(MSG_FRAME_END));
    bus_.Dispatch();
    frameArena_.Reset();
    scratch_.Reset();
}

void KoiloKernel::Tick(float dt) {
    modules_.TickAll(dt);
}

void KoiloKernel::Shutdown() {
    if (!running_) return;
    running_ = false;

    bus_.Send(MakeSignal(MSG_SHUTDOWN));
    bus_.Dispatch();

    modules_.ShutdownAll();
    pool_.Shutdown();
    // Clear services on shutdown
    for (auto& name : services_.List()) {
        services_.Unregister(name);
    }

    KL_LOG("Kernel", "Shutdown complete. Messages dispatched: %zu, dropped: %zu",
           bus_.TotalDispatched(), bus_.TotalDropped());
}

bool KoiloKernel::RequireCap(ModuleId caller, Cap required, const char* operation) {
#ifdef KL_BARE_METAL
    (void)caller; (void)required; (void)operation;
    return true;
#else
    if (modules_.HasCapability(caller, required)) return true;

    // Find module name for the warning
    const char* moduleName = "unknown";
    for (auto& m : modules_.ListModules()) {
        if (m.id == caller) { moduleName = m.name.c_str(); break; }
    }

    // Find cap name
    const char* capName = "unknown";
    for (int i = 0; i < ALL_CAPS_COUNT; ++i) {
        if (ALL_CAPS[i] == required) { capName = CapName(ALL_CAPS[i]); break; }
    }

    if (operation) {
        KL_WARN("Kernel", "Capability denied: module '%s' lacks '%s' for '%s'",
                moduleName, capName, operation);
    } else {
        KL_WARN("Kernel", "Capability denied: module '%s' lacks '%s'",
                moduleName, capName);
    }
    return false;
#endif
}

} // namespace koilo
