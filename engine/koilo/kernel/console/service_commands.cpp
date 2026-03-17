// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/kernel.hpp>
#include <sstream>

namespace koilo {

void RegisterServiceCommands(CommandRegistry& registry) {
    registry.Register({"services", "services", "List all registered kernel services",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto names = kernel.Services().List();
            if (names.empty())
                return ConsoleResult::Ok("No services registered.");

            std::ostringstream os;
            os << names.size() << " service(s):\n";
            for (auto& n : names)
                os << "  " << n << "\n";
            return ConsoleResult::Ok(os.str());
        }, nullptr
    });
}

} // namespace koilo
