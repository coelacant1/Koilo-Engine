// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file command_provider.hpp
 * @brief Interface for modules to declare console commands without
 *        coupling to ConsoleModule internals.
 *
 * Modules implement ICommandProvider and register as a
 * "commands.<module_name>" typed service. ConsoleModule discovers
 * all providers via ServiceRegistry prefix scan at init.
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once
#include <koilo/kernel/console/command_registry.hpp>
#include <vector>

namespace koilo {

/// Extension point: modules return their command definitions
/// without touching ConsoleModule or CommandRegistry directly.
class ICommandProvider {
public:
    virtual ~ICommandProvider() = default;

    /// Return all commands this module provides.
    virtual std::vector<CommandDef> GetCommands() const = 0;
};

} // namespace koilo
