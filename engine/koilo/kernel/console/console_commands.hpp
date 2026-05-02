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

/// Register entity/scene inspection commands (entity.list, entity.inspect, scene.hierarchy, etc.).
void RegisterEntityCommands(CommandRegistry& registry);

/// Register render graph commands (render.graph, render.passes, render.toggle).
void RegisterRenderGraphCommands(CommandRegistry& registry);

/// Register hot-reload commands (shader.list, shader.reload, exec.reload, hotreload.*).
void RegisterHotReloadCommands(CommandRegistry& registry);

/// Register GPU timing commands (stat.gpu, profile.capture).
void RegisterGPUTimingCommands(CommandRegistry& registry);

/// Register error reporting commands (errors, errors clear).
void RegisterErrorCommands(CommandRegistry& registry);

/// Register task inspection commands (task list).
void RegisterTaskCommands(CommandRegistry& registry);

/// Register module fault isolation commands (module status, module restart).
void RegisterModuleFaultCommands(CommandRegistry& registry);

/// Register schema versioning commands (schema list, schema check).
void RegisterSchemaCommands(CommandRegistry& registry);

/// Register screenshot capture command (screenshot <path>).
void RegisterScreenshotCommands(CommandRegistry& registry);

/// Tick the frame-capture system (call once per frame from host loop).
void TickProfileCapture(KoiloKernel& kernel, int frameNumber);

/// Try interpreting a command as a CVar name (get/set fallback).
ConsoleResult TryCVarFallback(const std::string& name, const std::vector<std::string>& args);

} // namespace koilo
