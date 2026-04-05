// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/console/command_registry.hpp>
#include <koilo/kernel/console/console_result.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/module_manager.hpp>
#include <cstdio>

namespace koilo {

static const char* ModuleStateName(ModuleState s) {
    switch (s) {
        case ModuleState::Registered:   return "registered";
        case ModuleState::Resolving:    return "resolving";
        case ModuleState::Initialized:  return "initialized";
        case ModuleState::Running:      return "running";
        case ModuleState::Faulted:      return "FAULTED";
        case ModuleState::ShuttingDown: return "shutting_down";
        case ModuleState::Unloaded:     return "unloaded";
        case ModuleState::Error:        return "ERROR";
        default:                        return "unknown";
    }
}

void RegisterModuleFaultCommands(CommandRegistry& registry) {
    registry.Register({
        "module",
        "module status | module restart <name>",
        "Module fault status and restart commands.",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty()) {
                return ConsoleResult::Error("Usage: module status | module restart <name>");
            }

            const auto& sub = args[0];

            if (sub == "status") {
                auto& mgr = kernel.Modules();
                auto modules = mgr.ListModules();

                if (modules.empty()) {
                    return ConsoleResult::Ok("No modules registered.");
                }

                std::string out;
                for (auto& m : modules) {
                    char line[256];
                    std::snprintf(line, sizeof(line), "  %-24s %s\n",
                                  m.name.c_str(), ModuleStateName(m.state));
                    out += line;
                }
                return ConsoleResult::Ok(out);
            }

            if (sub == "restart") {
                if (args.size() < 2) {
                    return ConsoleResult::Error("Usage: module restart <name>");
                }
                const auto& name = args[1];
                if (kernel.Modules().RestartModule(name)) {
                    return ConsoleResult::Ok("Module '" + name + "' restarted.");
                }
                return ConsoleResult::Error(
                    "Failed to restart '" + name + "' (not faulted or not found).");
            }

            return ConsoleResult::Error("Unknown subcommand: " + sub);
        },
        // Tab completion
        [](KoiloKernel& kernel, const std::vector<std::string>& args,
           const std::string&) -> std::vector<std::string> {
            if (args.size() <= 1) {
                return {"status", "restart"};
            }
            if (args.size() == 2 && args[0] == "restart") {
                auto modules = kernel.Modules().ListModules();
                std::vector<std::string> names;
                for (auto& m : modules) {
                    if (m.state == ModuleState::Faulted) {
                        names.push_back(m.name);
                    }
                }
                return names;
            }
            return {};
        }
    });
}

} // namespace koilo
