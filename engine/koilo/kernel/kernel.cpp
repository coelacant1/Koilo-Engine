#include <koilo/kernel/kernel.hpp>
#include <cstdio>

namespace koilo {

KoiloKernel::KoiloKernel()
    : KoiloKernel(Config{}) {
}

KoiloKernel::KoiloKernel(const Config& config)
    : frameArena_(config.frameArenaSize)
    , scratch_(config.scratchSize)
    , bus_(config.messageBusSize)
    , modules_(bus_, services_) {
}

KoiloKernel::~KoiloKernel() {
    if (running_) {
        Shutdown();
    }
}

ModuleId KoiloKernel::RegisterModule(const ModuleDesc& desc, Cap caps) {
    return modules_.RegisterModule(desc, caps);
}

bool KoiloKernel::InitializeModules() {
    if (!modules_.InitializeAll(*this)) {
        std::fprintf(stderr, "[Kernel] Module initialization failed\n");
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
    // Clear services on shutdown
    for (auto& name : services_.List()) {
        services_.Unregister(name);
    }

    std::fprintf(stderr, "[Kernel] Shutdown complete. Messages dispatched: %zu, dropped: %zu\n",
                 bus_.TotalDispatched(), bus_.TotalDropped());
}

} // namespace koilo
