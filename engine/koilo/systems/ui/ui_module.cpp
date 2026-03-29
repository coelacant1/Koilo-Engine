// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui_module.hpp"
#include "ui.hpp"
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/kernel/kernel.hpp>

namespace koilo {

UIModule::UIModule() = default;
UIModule::~UIModule() { Shutdown(); }

ModuleInfo UIModule::GetInfo() const {
    return {"ui", "0.1.0", ModulePhase::Overlay};
}

bool UIModule::Initialize(KoiloKernel& kernel) {
    kernel_ = &kernel;

    // Ensure reflection is registered before creating globals
    (void)UI::Describe();
    ui_ = std::make_unique<UI>();

    auto* engine = kernel.Services().Get<scripting::KoiloScriptEngine>("script");
    if (engine) {
        // Wire script callbacks so UI buttons can call script functions
        ui_->Context().SetScriptCallback([engine](const char* fnName) {
            engine->CallFunction(std::string(fnName));
        });
        engine->RegisterGlobal("ui", "UI", ui_.get());
    }
    kernel.Services().Register("ui", ui_.get());
    return true;
}

void UIModule::Update(float /*dt*/) {
    // UI animations are updated explicitly via UpdateAnimations
}

void UIModule::Render(Color888* buffer, int width, int height) {
    if (ui_) {
        ui_->RenderToBuffer(buffer, width, height);
    }
}

void UIModule::Shutdown() {
    if (kernel_) {
        kernel_->Services().Unregister("ui");
    }
    ui_.reset();
}

void UIModule::EnsureFont() {
    if (ui_ && !ui_->HasFont()) {
        ui_->LoadFont("assets/fonts/NotoSansMono-VariableFont_wdth,wght.ttf", 14.0f);
        ui_->LoadBoldFont("assets/fonts/NotoSansMono-Bold.ttf", 14.0f);
    }
}

void UIModule::UpdateAnimations(float dt) {
    if (!ui_) return;
    EnsureFont();
    ui_->UpdateAnimations(dt);
}

} // namespace koilo
