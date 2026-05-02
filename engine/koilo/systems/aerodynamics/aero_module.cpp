// SPDX-License-Identifier: GPL-3.0-or-later
#include "aero_module.hpp"

#include "aerodynamicsworld.hpp"
#include "windfield.hpp"

#include <koilo/kernel/kernel.hpp>
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/systems/physics/physicsworld.hpp>

namespace koilo::aero {

AerodynamicsModule::AerodynamicsModule() = default;

AerodynamicsModule::~AerodynamicsModule() {
    Shutdown();
}

ModuleInfo AerodynamicsModule::GetInfo() const {
    return {"aerodynamics", "0.1.0", ModulePhase::System};
}

bool AerodynamicsModule::Initialize(KoiloKernel& kernel) {
    kernel_ = &kernel;
    world_       = std::make_unique<AerodynamicsWorld>();
    defaultWind_ = std::make_unique<ConstantWind>();
    world_->SetWindField(defaultWind_.get());

    // Expose the live AerodynamicsWorld to script as `aero`. Mirrors the
    // PhysicsModule pattern (registers `physics`). Scripts can call e.g.
    // `aero.RegisterEngine(body, eng)`.
    if (auto* engine = kernel.Services().Get<scripting::KoiloScriptEngine>("script")) {
        engine->RegisterGlobal("aero", "AerodynamicsWorld", world_.get());
    }

    // discover the PhysicsWorld registered by PhysicsModule
    // (service name "physics.world"). Order: aero must be initialized
    // AFTER physics. If physics is unavailable the user can call
    // GetWorld()->AttachToPhysics(...) manually after physics comes up.
    auto* physWorld = kernel.Services().Get<PhysicsWorld>("physics.world");
    if (physWorld) {
        world_->AttachToPhysics(physWorld);
    }
    return true;
}

void AerodynamicsModule::Update(float /*dt*/) {
    // Forces are applied per physics substep via the pre-step callback.
}

void AerodynamicsModule::Shutdown() {
    if (world_) {
        world_->DetachFromPhysics();
    }
    world_.reset();
    defaultWind_.reset();
}

} // namespace koilo::aero
