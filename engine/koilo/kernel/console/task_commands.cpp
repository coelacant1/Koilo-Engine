// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file task_commands.cpp
 * @brief Console commands for inspecting active task groups.
 *
 * Commands:
 *   task list  - show active thread pool status
 *
 * @date 03/30/2026
 * @author Coela
 */
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/console/command_registry.hpp>
#include <koilo/kernel/console/console_result.hpp>
#include <koilo/kernel/kernel.hpp>

#include <sstream>

namespace koilo {

void RegisterTaskCommands(CommandRegistry& registry) {
    registry.Register({
        "task",
        "task list",
        "Show thread pool status.",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty() || args[0] != "list") {
                return ConsoleResult::Error("Usage: task list");
            }

            auto& pool = kernel.Pool();
            std::ostringstream out;
            out << "ThreadPool: " << pool.ThreadCount() << " workers, "
                << pool.PendingCount() << " pending, "
                << (pool.IsRunning() ? "running" : "stopped") << "\n";

            return ConsoleResult::Ok(out.str());
        },
        [](KoiloKernel&, const std::vector<std::string>&,
           const std::string& partial) -> std::vector<std::string> {
            if (std::string("list").find(partial) == 0)
                return {"list"};
            return {};
        }
    });
}

} // namespace koilo
