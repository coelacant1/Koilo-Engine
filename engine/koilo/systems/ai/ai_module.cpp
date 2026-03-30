// SPDX-License-Identifier: GPL-3.0-or-later
#include "ai_module.hpp"
#include "script_ai_manager.hpp"
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/kernel/kernel.hpp>

namespace koilo {

AIModule::AIModule() = default;

AIModule::~AIModule() {
    Shutdown();
}

ModuleInfo AIModule::GetInfo() const {
    return {"ai", "0.1.0", ModulePhase::System};
}

bool AIModule::Initialize(KoiloKernel& kernel) {
    kernel_ = &kernel;
    auto* engine = kernel.Services().Get<scripting::KoiloScriptEngine>("script");
    manager_ = std::make_unique<ScriptAIManager>();
    engine->RegisterGlobal("ai", "ScriptAIManager", manager_.get());

    kernel.Services().RegisterTyped<ICommandProvider>("commands.ai", this);

    return true;
}

std::vector<CommandDef> AIModule::GetCommands() const {
    std::vector<CommandDef> cmds;
    auto* mgr = manager_.get();
    cmds.push_back({"ai", "ai [status|machines|paths]", "AI system info",
        [mgr](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (!mgr) return ConsoleResult::Error("AI module not initialized");
            if (args.empty() || args[0] == "status") {
                return ConsoleResult::Ok("AI module: active");
            }
            return ConsoleResult::Error("Unknown subcommand: " + args[0]);
        }, nullptr
    });
    return cmds;
}

void AIModule::Update(float dt) {
    if (manager_) {
        manager_->Update();
    }
}

void AIModule::Render(Color888*, int, int) {
    // AI has no render pass
}

void AIModule::Shutdown() {
    manager_.reset();
}

} // namespace koilo
