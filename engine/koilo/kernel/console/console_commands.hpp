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

/// Register service registry and capability console commands (grant, revoke).
void RegisterServiceCommands(CommandRegistry& registry);

/// Register memory allocator console commands (memory).
void RegisterMemoryCommands(CommandRegistry& registry);

/// Register GPU device console commands (gpu.list, gpu.info).
void RegisterGPUCommands(CommandRegistry& registry);

/// Register console variable commands (cvar.list, cvar.set, cvar.reset).
void RegisterCVarCommands(CommandRegistry& registry);

/// Register structured logging commands (log.level, log.channels, log.tail, etc.).
void RegisterLogCommands(CommandRegistry& registry);

/// Register utility commands (exec, time, repeat, sleep, stat.*, watch, pause/step).
void RegisterUtilityCommands(CommandRegistry& registry);

/// Register config store commands (config.list, config.get, config.set, config.save, config.load).
void RegisterConfigCommands(CommandRegistry& registry);

/// Try interpreting a command as a CVar name (get/set fallback).
ConsoleResult TryCVarFallback(const std::string& name, const std::vector<std::string>& args);

} // namespace koilo
