// SPDX-License-Identifier: GPL-3.0-or-later
#include "particle_module.hpp"
#include "particlesystem.hpp"
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/kernel/kernel.hpp>

namespace koilo {

ParticleModule::ParticleModule() = default;
ParticleModule::~ParticleModule() { Shutdown(); }

ModuleInfo ParticleModule::GetInfo() const {
    return {"particles", "0.1.0", ModulePhase::Render};
}

bool ParticleModule::Initialize(KoiloKernel& kernel) {
    kernel_ = &kernel;
    system_ = std::make_unique<ParticleSystem>();

    auto* engine = kernel.Services().Get<scripting::KoiloScriptEngine>("script");
    if (engine) {
        engine->RegisterGlobal("particles", "ParticleSystem", system_.get());
    }
    kernel.Services().Register("particles", system_.get());
    return true;
}

void ParticleModule::Update(float dt) {
    (void)dt;
    if (system_) system_->Update();
}

void ParticleModule::Render(Color888* buffer, int width, int height) {
    if (system_) {
        system_->Render(buffer, width, height);
    }
}

void ParticleModule::Shutdown() {
    if (kernel_) {
        kernel_->Services().Unregister("particles");
    }
    system_.reset();
}

} // namespace koilo
