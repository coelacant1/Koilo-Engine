// SPDX-License-Identifier: GPL-3.0-or-later
#include "audio_module.hpp"
#include "script_audio_manager.hpp"
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/kernel/kernel.hpp>

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

    kernel.Services().RegisterTyped<ICommandProvider>("commands.audio", this);

    return true;
}

std::vector<CommandDef> AudioModule::GetCommands() const {
    std::vector<CommandDef> cmds;
    auto* mgr = manager_.get();
    cmds.push_back({"audio", "audio [status|stop]", "Audio system info and control",
        [mgr](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (!mgr) return ConsoleResult::Error("Audio module not initialized");
            if (args.empty() || args[0] == "status") {
                return ConsoleResult::Ok("Audio module: active");
            }
            if (args[0] == "stop") {
                mgr->StopAll();
                return ConsoleResult::Ok("All audio stopped.");
            }
            return ConsoleResult::Error("Unknown subcommand: " + args[0]);
        }, nullptr
    });
    return cmds;
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
