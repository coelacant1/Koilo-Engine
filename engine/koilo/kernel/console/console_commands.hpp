// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file console_commands.hpp
 * @brief Built-in console command registration helpers.
 *
 * @date 01/04/2026
 * @author Coela
 */
#pragma once
#include <koilo/kernel/console/command_registry.hpp>

namespace koilo {

/// Register profiling-related console commands (perf, mem-report).
void RegisterProfilingCommands(CommandRegistry& registry);

/// Register reflection-related console commands (classes, inspect, get, set).
void RegisterReflectionCommands(CommandRegistry& registry);

/// Register message bus console commands (channels, tap, send).
void RegisterMessageCommands(CommandRegistry& registry);

/// Register service registry console commands (services).
void RegisterServiceCommands(CommandRegistry& registry);

/// Register memory allocator console commands (memory).
void RegisterMemoryCommands(CommandRegistry& registry);

} // namespace koilo
