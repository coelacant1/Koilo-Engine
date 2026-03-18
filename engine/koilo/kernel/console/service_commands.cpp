// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <sstream>

namespace koilo {

void RegisterServiceCommands(CommandRegistry& registry) {
    // -- grant --
    registry.Register({"grant", "grant <module> <cap>",
        "Grant a capability to a module (e.g. grant koilo.audio gpu.access)",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.size() < 2) return ConsoleResult::Error("Usage: grant <module> <cap>");
            ModuleId id = kernel.Modules().GetId(args[0]);
            if (id == 0) return ConsoleResult::NotFound("Module not found: " + args[0]);
            // Find cap by name
            for (int i = 0; i < ALL_CAPS_COUNT; ++i) {
                if (args[1] == CapName(ALL_CAPS[i])) {
                    kernel.Modules().GrantCap(id, ALL_CAPS[i]);
                    KL_LOG("Kernel", "Granted %s to module %s", args[1].c_str(), args[0].c_str());
                    return ConsoleResult::Ok("Granted " + args[1] + " to " + args[0]);
                }
            }
            return ConsoleResult::Error("Unknown capability: " + args[1] +
                "\nAvailable: memory.alloc, file.read, file.write, gpu.access, network, module.load, console.admin, debug");
        },
        // Tab completer for cap names
        [](KoiloKernel& kernel, const std::vector<std::string>& args, const std::string& partial) -> std::vector<std::string> {
            std::vector<std::string> matches;
            if (args.empty()) {
                // Complete module names
                for (auto& m : kernel.Modules().ListModules()) {
                    if (m.name.find(partial) == 0) matches.push_back(m.name);
                }
            } else {
                // Complete cap names
                for (int i = 0; i < ALL_CAPS_COUNT; ++i) {
                    std::string name = CapName(ALL_CAPS[i]);
                    if (name.find(partial) == 0) matches.push_back(name);
                }
            }
            return matches;
        }
    });

    // -- revoke --
    registry.Register({"revoke", "revoke <module> <cap>",
        "Revoke a capability from a module",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.size() < 2) return ConsoleResult::Error("Usage: revoke <module> <cap>");
            ModuleId id = kernel.Modules().GetId(args[0]);
            if (id == 0) return ConsoleResult::NotFound("Module not found: " + args[0]);
            for (int i = 0; i < ALL_CAPS_COUNT; ++i) {
                if (args[1] == CapName(ALL_CAPS[i])) {
                    kernel.Modules().RevokeCap(id, ALL_CAPS[i]);
                    KL_WARN("Kernel", "Revoked %s from module %s", args[1].c_str(), args[0].c_str());
                    return ConsoleResult::Ok("Revoked " + args[1] + " from " + args[0]);
                }
            }
            return ConsoleResult::Error("Unknown capability: " + args[1]);
        },
        // Same completer as grant
        [](KoiloKernel& kernel, const std::vector<std::string>& args, const std::string& partial) -> std::vector<std::string> {
            std::vector<std::string> matches;
            if (args.empty()) {
                for (auto& m : kernel.Modules().ListModules()) {
                    if (m.name.find(partial) == 0) matches.push_back(m.name);
                }
            } else {
                for (int i = 0; i < ALL_CAPS_COUNT; ++i) {
                    std::string name = CapName(ALL_CAPS[i]);
                    if (name.find(partial) == 0) matches.push_back(name);
                }
            }
            return matches;
        }
    });
}

} // namespace koilo
