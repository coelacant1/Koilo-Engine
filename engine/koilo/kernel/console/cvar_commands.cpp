// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/cvar/cvar_system.hpp>
#include <sstream>
#include <iomanip>

namespace koilo {

void RegisterCVarCommands(CommandRegistry& registry) {
    // -- cvar.list --
    registry.Register({"cvar.list", "cvar.list [prefix]", "List console variables (optionally filtered by prefix)",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            auto& sys = CVarSystem::Get();
            std::string prefix = args.empty() ? "" : args[0];

            std::ostringstream os;
            int count = 0;

            auto print = [&](const CVarParameter& p) {
                if (!prefix.empty() && p.name.compare(0, prefix.size(), prefix) != 0)
                    return;
                if (HasFlag(p.flags, CVarFlags::Advanced) && prefix.empty())
                    return;  // hide advanced cvars in unfiltered listing
                os << "  " << std::left << std::setw(28) << p.name
                   << " = " << std::setw(12) << p.ValueString()
                   << " " << p.description << "\n";
                ++count;
            };

            sys.ForEach(print);

            if (count == 0) {
                return ConsoleResult::Ok(prefix.empty()
                    ? "No CVars registered."
                    : "No CVars matching prefix '" + prefix + "'.");
            }

            std::ostringstream header;
            header << count << " CVar(s)";
            if (!prefix.empty()) header << " matching '" << prefix << "'";
            header << ":\n" << os.str();
            return ConsoleResult::Ok(header.str());
        },
        // Tab-completer: complete cvar prefixes
        [](KoiloKernel&, const std::vector<std::string>& args, const std::string& partial) -> std::vector<std::string> {
            std::vector<std::string> matches;
            CVarSystem::Get().ForEach([&](const CVarParameter& p) {
                if (p.name.compare(0, partial.size(), partial) == 0)
                    matches.push_back(p.name);
            });
            return matches;
        }
    });

    // -- cvar.set --
    registry.Register({"cvar.set", "cvar.set <name> <value>", "Set a console variable",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.size() < 2)
                return ConsoleResult::Error("Usage: cvar.set <name> <value>");

            auto* p = CVarSystem::Get().Find(args[0]);
            if (!p)
                return ConsoleResult::Error("Unknown CVar: " + args[0]);

            if (HasFlag(p->flags, CVarFlags::ReadOnly))
                return ConsoleResult::Error("CVar '" + args[0] + "' is read-only.");

            // Join remaining args as value (for strings with spaces)
            std::string val = args[1];
            for (size_t i = 2; i < args.size(); ++i)
                val += " " + args[i];

            if (!p->SetFromString(val))
                return ConsoleResult::Error("Failed to parse '" + val + "' for CVar '" + args[0] + "'.");

            return ConsoleResult::Ok(args[0] + " = " + p->ValueString());
        },
        // Tab-completer: complete cvar names
        [](KoiloKernel&, const std::vector<std::string>&, const std::string& partial) -> std::vector<std::string> {
            std::vector<std::string> matches;
            CVarSystem::Get().ForEach([&](const CVarParameter& p) {
                if (p.name.compare(0, partial.size(), partial) == 0)
                    matches.push_back(p.name);
            });
            return matches;
        }
    });

    // -- cvar.reset --
    registry.Register({"cvar.reset", "cvar.reset <name>", "Reset a console variable to its default value",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty())
                return ConsoleResult::Error("Usage: cvar.reset <name>");

            auto* p = CVarSystem::Get().Find(args[0]);
            if (!p)
                return ConsoleResult::Error("Unknown CVar: " + args[0]);

            p->ResetToDefault();
            return ConsoleResult::Ok(args[0] + " reset to " + p->ValueString());
        }, nullptr
    });
}

// -- CVar fallback for the console session --
// When a command is not found, check if the input matches a CVar name.
// If so, print or set it.

ConsoleResult TryCVarFallback(const std::string& name, const std::vector<std::string>& args) {
    auto* p = CVarSystem::Get().Find(name);
    if (!p) return ConsoleResult::Error("");  // not a cvar either

    if (args.empty()) {
        // Just the name: print value + description
        std::ostringstream os;
        os << name << " = " << p->ValueString()
           << "  (default: " << p->DefaultString() << ")\n"
           << "  " << p->description;
        return ConsoleResult::Ok(os.str());
    }

    // Name + value: set it
    if (HasFlag(p->flags, CVarFlags::ReadOnly))
        return ConsoleResult::Error("CVar '" + name + "' is read-only.");

    std::string val = args[0];
    for (size_t i = 1; i < args.size(); ++i)
        val += " " + args[i];

    if (!p->SetFromString(val))
        return ConsoleResult::Error("Failed to parse '" + val + "' for CVar '" + name + "'.");

    return ConsoleResult::Ok(name + " = " + p->ValueString());
}

} // namespace koilo
