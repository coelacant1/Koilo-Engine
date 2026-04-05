// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file schema_commands.cpp
 * @brief Console commands for schema versioning inspection.
 */
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/schema_version.hpp>
#include <koilo/kernel/schema_migrator.hpp>
#include <sstream>
#include <fstream>

namespace koilo {

void RegisterSchemaCommands(CommandRegistry& registry) {
    registry.Register({
        "schema list",
        "schema list",
        "List registered schema formats and migration versions",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto& migrator = kernel.Migrator();
            auto formats = migrator.Formats();
            if (formats.empty()) {
                return ConsoleResult::Ok("No schema migrations registered.");
            }
            std::ostringstream ss;
            ss << "Registered schema formats:\n";
            for (auto& fmt : formats) {
                uint32_t latest = migrator.LatestVersion(fmt);
                ss << "  " << fmt << " (latest: v" << latest << ")\n";
            }
            return ConsoleResult::Ok(ss.str());
        },
        nullptr
    });

    registry.Register({
        "schema check",
        "schema check <file>",
        "Read and display the schema header of a binary file",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.size() < 2) {
                return ConsoleResult::Error("Usage: schema check <file>");
            }
            std::ifstream f(args[1], std::ios::binary);
            if (!f.is_open()) {
                return ConsoleResult::Error("Cannot open file: " + args[1]);
            }
            char magic[4] = {};
            f.read(magic, 4);
            uint32_t version = 0;
            f.read(reinterpret_cast<char*>(&version), sizeof(version));
            if (!f.good()) {
                return ConsoleResult::Error("File too small to contain a schema header.");
            }
            std::ostringstream ss;
            ss << "File: " << args[1] << "\n";
            ss << "  Magic: " << std::string(magic, 4) << "\n";
            ss << "  Version: " << version << "\n";
            return ConsoleResult::Ok(ss.str());
        },
        nullptr
    });
}

} // namespace koilo
