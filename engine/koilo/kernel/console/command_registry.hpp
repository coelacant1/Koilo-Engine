// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file command_registry.hpp
 * @brief Registry for console commands with tab-completion support.
 *
 * @date 12/11/2025
 * @author Coela
 */
#pragma once
#include <koilo/kernel/console/console_result.hpp>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

class KoiloKernel;

/// Describes a registered console command.
struct CommandDef {
    std::string name;
    std::string usage;       // e.g. "mem [module]"
    std::string description; // one-line help
    std::function<ConsoleResult(KoiloKernel&, const std::vector<std::string>&)> handler;
    std::function<std::vector<std::string>(KoiloKernel&, const std::vector<std::string>&, const std::string&)> completer;

    KL_BEGIN_FIELDS(CommandDef)
        KL_FIELD(CommandDef, name, "Name", 0, 0),
        KL_FIELD(CommandDef, usage, "Usage", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(CommandDef)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CommandDef)
        /* No reflected ctors. */
    KL_END_DESCRIBE(CommandDef)

};

/// Stores and dispatches console commands.
class CommandRegistry {
public:
    CommandRegistry() = default;

    /// Register a command.
    void Register(CommandDef def);

    /// Execute a command by name with arguments.
    ConsoleResult Execute(KoiloKernel& kernel, const std::string& name,
                          const std::vector<std::string>& args) const;

    /// Get tab-completion candidates for partial input.
    std::vector<std::string> Complete(KoiloKernel& kernel, const std::string& name,
                                      const std::vector<std::string>& args,
                                      const std::string& partial) const;

    /// Get a command definition by name (nullptr if not found).
    const CommandDef* Find(const std::string& name) const;

    /// List all registered command names.
    std::vector<std::string> ListCommands() const;

    /// Get all command definitions (for help listing).
    const std::unordered_map<std::string, CommandDef>& All() const { return commands_; }

private:
    std::unordered_map<std::string, CommandDef> commands_;

    KL_BEGIN_FIELDS(CommandRegistry)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(CommandRegistry)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(CommandRegistry)
        KL_CTOR0(CommandRegistry)
    KL_END_DESCRIBE(CommandRegistry)

};

} // namespace koilo
