// SPDX-License-Identifier: GPL-3.0-or-later
#include "input_module.hpp"
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/kernel/kernel.hpp>

namespace koilo {

InputModule::InputModule() = default;
InputModule::~InputModule() = default;

ModuleInfo InputModule::GetInfo() const {
    return {"input", "0.1.0", ModulePhase::Core};
}

bool InputModule::Initialize(KoiloKernel& kernel) {
    kernel_ = &kernel;

    auto* engine = kernel.Services().Get<scripting::KoiloScriptEngine>("script");
    if (engine) {
        engine->RegisterGlobal("input", "InputManager", &manager_);
    }
    kernel.Services().Register("input", &manager_);
    return true;
}

void InputModule::Update(float dt) {
    (void)dt;
    manager_.Update();
}

void InputModule::Shutdown() {
    if (kernel_) {
        kernel_->Services().Unregister("input");
    }
}

} // namespace koilo
