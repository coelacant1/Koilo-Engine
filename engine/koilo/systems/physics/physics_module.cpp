// SPDX-License-Identifier: GPL-3.0-or-later
#include "physics_module.hpp"
#include "physicsworld.hpp"
#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/console/console_module.hpp>
#include <sstream>
#include <iomanip>

namespace koilo {

PhysicsModule::PhysicsModule() = default;

PhysicsModule::~PhysicsModule() {
    Shutdown();
}

ModuleInfo PhysicsModule::GetInfo() const {
    return {"physics", "0.1.0", ModulePhase::System};
}

bool PhysicsModule::Initialize(KoiloKernel& kernel) {
    kernel_ = &kernel;
    auto* engine = kernel.Services().Get<scripting::KoiloScriptEngine>("script");
    world_ = std::make_unique<PhysicsWorld>();
    engine->RegisterGlobal("physics", "PhysicsWorld", world_.get());

    // Wire collision callbacks to script engine
    world_->OnCollisionEnter([engine](const CollisionEvent&) {
        engine->CallFunction("OnCollisionEnter");
    });
    world_->OnCollisionExit([engine](const CollisionEvent&) {
        engine->CallFunction("OnCollisionExit");
    });

    // Register console commands if available
    auto* console = ConsoleModule::Instance();
    if (console) {
        auto& cmds = console->Commands();
        cmds.Register({"physics", "physics [status|gravity|bodies]", "Physics system info and control",
            [this](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
                if (!world_) return ConsoleResult::Error("Physics module not initialized");
                if (args.empty() || args[0] == "status") {
                    auto g = world_->GetGravity();
                    std::ostringstream ss;
                    ss << "Physics module: active\n"
                       << "  Bodies: " << world_->GetBodyCount() << "\n"
                       << std::fixed << std::setprecision(2)
                       << "  Gravity: (" << g.X << ", " << g.Y << ", " << g.Z << ")\n"
                       << "  Timestep: " << world_->GetFixedTimestep() << "s";
                    return ConsoleResult::Ok(ss.str());
                }
                if (args[0] == "gravity" && args.size() >= 4) {
                    try {
                        float x = std::stof(args[1]);
                        float y = std::stof(args[2]);
                        float z = std::stof(args[3]);
                        world_->SetGravity(Vector3D(x, y, z));
                        return ConsoleResult::Ok("Gravity set to (" +
                            args[1] + ", " + args[2] + ", " + args[3] + ")");
                    } catch (...) {
                        return ConsoleResult::Error("Usage: physics gravity <x> <y> <z>");
                    }
                }
                if (args[0] == "bodies") {
                    return ConsoleResult::Ok("Body count: " + std::to_string(world_->GetBodyCount()));
                }
                return ConsoleResult::Error("Unknown subcommand: " + args[0]);
            }, nullptr
        });
    }

    return true;
}

void PhysicsModule::Update(float dt) {
    if (world_) {
        world_->Step();
    }
}

void PhysicsModule::Render(Color888*, int, int) {
    // Physics has no render pass
}

void PhysicsModule::Shutdown() {
    world_.reset();
}

} // namespace koilo
