// SPDX-License-Identifier: GPL-3.0-or-later
#include "render_module.hpp"
#include "irenderbackend.hpp"
#include "igpu_render_backend.hpp"
#include <koilo/kernel/kernel.hpp>

namespace koilo {

RenderModule::RenderModule() = default;
RenderModule::~RenderModule() { Shutdown(); }

ModuleInfo RenderModule::GetInfo() const {
    return {"render", "0.1.0", ModulePhase::Render};
}

bool RenderModule::Initialize(KoiloKernel& kernel) {
    kernel_ = &kernel;
    // Backend is set later via SetBackend() from the host
    return true;
}

void RenderModule::Update(float /*dt*/) {
    // No per-frame work - rendering is driven explicitly
}

void RenderModule::Shutdown() {
    if (kernel_ && backend_) {
        kernel_->Services().Unregister("render_backend");
    }
    backend_.reset();
}

void RenderModule::SetBackend(std::unique_ptr<IRenderBackend> backend) {
    // Unregister previous
    if (kernel_ && backend_ && kernel_->Services().Has("render_backend")) {
        kernel_->Services().Unregister("render_backend");
    }
    backend_ = std::move(backend);
    if (backend_ && !backend_->IsInitialized()) {
        backend_->Initialize();
    }
    // Register new
    if (kernel_ && backend_) {
        kernel_->Services().Register("render_backend", backend_.get());
    }
}

} // namespace koilo
