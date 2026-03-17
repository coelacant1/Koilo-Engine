// SPDX-License-Identifier: GPL-3.0-or-later
#include "audio_module.hpp"
#include "script_audio_manager.hpp"
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/console/console_module.hpp>

namespace koilo {

AudioModule::AudioModule() = default;

AudioModule::~AudioModule() {
    Shutdown();
}

ModuleInfo AudioModule::GetInfo() const {
    return {"audio", "0.1.0", ModulePhase::System};
}

bool AudioModule::Initialize(KoiloKernel& kernel) {
    kernel_ = &kernel;
    auto* engine = kernel.Services().Get<scripting::KoiloScriptEngine>("script");
    manager_ = std::make_unique<ScriptAudioManager>();
    engine->RegisterGlobal("audio", "ScriptAudioManager", manager_.get());

    // Register as kernel service if available
    auto* console = ConsoleModule::Instance();
    if (console) {
        auto& cmds = console->Commands();
        cmds.Register({"audio", "audio [status|stop]", "Audio system info and control",
            [this](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
                if (!manager_) return ConsoleResult::Error("Audio module not initialized");
                if (args.empty() || args[0] == "status") {
                    return ConsoleResult::Ok("Audio module: active");
                }
                if (args[0] == "stop") {
                    manager_->StopAll();
                    return ConsoleResult::Ok("All audio stopped.");
                }
                return ConsoleResult::Error("Unknown subcommand: " + args[0]);
            }, nullptr
        });
    }

    return true;
}

void AudioModule::Update(float dt) {
    if (manager_) {
        manager_->Update();
    }
}

void AudioModule::Render(Color888*, int, int) {
    // Audio has no render pass
}

void AudioModule::Shutdown() {
    manager_.reset();
}

} // namespace koilo
