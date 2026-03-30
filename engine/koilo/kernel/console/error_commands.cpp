// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file error_commands.cpp
 * @brief Console commands for inspecting structured engine errors.
 *
 * Commands:
 *   errors                         - show recent errors
 *   errors --severity <level>      - filter by severity
 *   errors --system <name>         - filter by subsystem
 *   errors clear                   - flush error history
 *
 * @date 03/30/2026
 * @author Coela
 */
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/console/command_registry.hpp>
#include <koilo/kernel/console/console_result.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/engine_error.hpp>

#include <string>
#include <sstream>
#include <vector>

namespace koilo {

void RegisterErrorCommands(CommandRegistry& registry) {
    registry.Register({
        "errors",
        "errors [clear] [--severity <level>] [--system <name>]",
        "Show recent engine errors or clear error history.",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            auto& errors = kernel.Errors();

            // errors clear
            if (!args.empty() && args[0] == "clear") {
                size_t count = errors.Count();
                errors.Clear();
                return ConsoleResult::Ok("Cleared " + std::to_string(count) + " error(s).");
            }

            // Parse optional filters
            const ErrorSeverity* sevFilter = nullptr;
            const ErrorSystem* sysFilter = nullptr;
            ErrorSeverity sevVal{};
            ErrorSystem sysVal{};

            for (size_t i = 0; i < args.size(); ++i) {
                if (args[i] == "--severity" && i + 1 < args.size()) {
                    sevVal = ParseErrorSeverity(args[i + 1].c_str());
                    sevFilter = &sevVal;
                    ++i;
                } else if (args[i] == "--system" && i + 1 < args.size()) {
                    sysVal = ParseErrorSystem(args[i + 1].c_str());
                    sysFilter = &sysVal;
                    ++i;
                }
            }

            auto results = errors.Query(sevFilter, sysFilter);

            if (results.empty()) {
                return ConsoleResult::Ok("No errors recorded.");
            }

            std::ostringstream out;
            out << results.size() << " error(s):\n";
            for (const auto* e : results) {
                out << "  [" << ErrorSeverityName(e->severity) << "] "
                    << ErrorSystemName(e->system) << "/"
                    << e->code << ": " << e->message;
                if (e->suggestion[0] != '\0') {
                    out << " (hint: " << e->suggestion << ")";
                }
                out << "\n";
            }

            return ConsoleResult::Ok(out.str());
        },
        [](KoiloKernel&, const std::vector<std::string>& args,
           const std::string& partial) -> std::vector<std::string> {
            std::vector<std::string> matches;

            // Complete subcommands
            if (args.empty()) {
                for (const char* opt : {"clear", "--severity", "--system"}) {
                    if (std::string(opt).find(partial) == 0)
                        matches.push_back(opt);
                }
                return matches;
            }

            // Complete --severity values
            const auto& prev = args.back();
            if (prev == "--severity") {
                for (const char* s : {"fatal", "degraded", "recoverable", "diagnostic"}) {
                    if (std::string(s).find(partial) == 0)
                        matches.push_back(s);
                }
                return matches;
            }

            // Complete --system values
            if (prev == "--system") {
                for (const char* s : {"kernel", "render", "asset", "script", "audio",
                                      "physics", "input", "ui", "ecs", "network", "module"}) {
                    if (std::string(s).find(partial) == 0)
                        matches.push_back(s);
                }
                return matches;
            }

            // Complete flags
            for (const char* opt : {"--severity", "--system", "clear"}) {
                if (std::string(opt).find(partial) == 0)
                    matches.push_back(opt);
            }
            return matches;
        }
    });
}

} // namespace koilo
