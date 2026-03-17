// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/kernel.hpp>
#include <sstream>
#include <iomanip>

namespace koilo {

static std::string FormatBytes(size_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024)
        return std::to_string(bytes / 1024) + " KB";
    double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << mb << " MB";
    return os.str();
}

void RegisterMemoryCommands(CommandRegistry& registry) {
    registry.Register({"memory", "memory [arena|scratch|budget]",
        "Show kernel memory allocator statistics",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            std::string sub = args.empty() ? "all" : args[0];

            auto& arena   = kernel.FrameArena();
            auto& scratch = kernel.Scratch();

            if (sub == "arena" || sub == "all") {
                std::ostringstream os;
                os << "Frame Arena:\n"
                   << "  Capacity:  " << FormatBytes(arena.Capacity()) << "\n"
                   << "  Used:      " << FormatBytes(arena.Used()) << "\n"
                   << "  Remaining: " << FormatBytes(arena.Remaining()) << "\n";
                if (sub == "arena") return ConsoleResult::Ok(os.str());
                // fall through for 'all'
                if (sub == "all") {
                    std::ostringstream os2;
                    os2 << os.str()
                        << "\nScratch (Linear):\n"
                        << "  Capacity:  " << FormatBytes(scratch.Capacity()) << "\n"
                        << "  Used:      " << FormatBytes(scratch.Used()) << "\n"
                        << "  Remaining: " << FormatBytes(scratch.Remaining()) << "\n";
                    return ConsoleResult::Ok(os2.str());
                }
            }

            if (sub == "scratch") {
                std::ostringstream os;
                os << "Scratch (Linear):\n"
                   << "  Capacity:  " << FormatBytes(scratch.Capacity()) << "\n"
                   << "  Used:      " << FormatBytes(scratch.Used()) << "\n"
                   << "  Remaining: " << FormatBytes(scratch.Remaining()) << "\n";
                return ConsoleResult::Ok(os.str());
            }

            if (sub == "budget") {
                std::ostringstream os;
                size_t total = arena.Capacity() + scratch.Capacity();
                size_t used  = arena.Used() + scratch.Used();
                os << "Memory Budget:\n"
                   << "  Total allocated: " << FormatBytes(total) << "\n"
                   << "  Total used:      " << FormatBytes(used) << "\n"
                   << "  Utilization:     "
                   << std::fixed << std::setprecision(1)
                   << (total > 0 ? 100.0 * used / total : 0.0) << "%\n";
                return ConsoleResult::Ok(os.str());
            }

            return ConsoleResult::Error("Unknown subcommand: " + sub +
                "\nUsage: memory [arena|scratch|budget]");
        }, nullptr
    });
}

} // namespace koilo
