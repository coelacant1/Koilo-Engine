// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file utility_commands.cpp
 * @brief Console utility commands: exec, time, stat.*, repeat, sleep.
 *
 * @date 01/15/2026
 * @author Coela Can't
 */

#include "console_commands.hpp"
#include "console_module.hpp"
#include "../logging/log.hpp"
#include "../sim_cvars.hpp"
#include "../debug_overlay.hpp"
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/cvar/cvar_system.hpp>
#include <koilo/systems/profiling/performanceprofiler.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <algorithm>

namespace koilo {

void RegisterUtilityCommands(CommandRegistry& registry) {

    // ---------------------------------------------------------------
    // exec <file> - run commands from a text file
    // ---------------------------------------------------------------
    registry.Register({"exec", "exec <file>",
        "Execute commands from a text file (one per line, # = comment)",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty()) return ConsoleResult::Error("Usage: exec <file>");

            auto* console = ConsoleModule::Instance();
            if (!console) return ConsoleResult::Error("Console not available");

            std::ifstream file(args[0]);
            if (!file.is_open())
                return ConsoleResult::Error("Cannot open file: " + args[0]);

            std::ostringstream output;
            std::string line;
            int executed = 0, skipped = 0;
            while (std::getline(file, line)) {
                // Trim leading whitespace
                size_t start = line.find_first_not_of(" \t");
                if (start == std::string::npos) { ++skipped; continue; }
                line = line.substr(start);

                // Skip comments and empty lines
                if (line.empty() || line[0] == '#') { ++skipped; continue; }

                auto result = console->Execute(line);
                if (!result.text.empty())
                    output << result.text << "\n";
                ++executed;
            }
            output << "--- exec: " << executed << " commands executed, "
                   << skipped << " lines skipped ---";
            return ConsoleResult::Ok(output.str());
        }, nullptr
    });

    // ---------------------------------------------------------------
    // time <command ...> - run a command and report execution time
    // ---------------------------------------------------------------
    registry.Register({"time", "time <command ...>",
        "Run a command and report execution time",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty()) return ConsoleResult::Error("Usage: time <command ...>");

            auto* console = ConsoleModule::Instance();
            if (!console) return ConsoleResult::Error("Console not available");

            // Reconstruct the full command line
            std::string cmdLine;
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) cmdLine += ' ';
                cmdLine += args[i];
            }

            auto t0 = std::chrono::high_resolution_clock::now();
            auto result = console->Execute(cmdLine);
            auto t1 = std::chrono::high_resolution_clock::now();

            double us = std::chrono::duration<double, std::micro>(t1 - t0).count();

            std::ostringstream os;
            if (!result.text.empty())
                os << result.text << "\n";

            if (us < 1000.0)
                os << "--- " << std::fixed << std::setprecision(1) << us << " µs ---";
            else if (us < 1000000.0)
                os << "--- " << std::fixed << std::setprecision(2) << (us / 1000.0) << " ms ---";
            else
                os << "--- " << std::fixed << std::setprecision(3) << (us / 1000000.0) << " s ---";

            return ConsoleResult::Ok(os.str());
        }, nullptr
    });

    // ---------------------------------------------------------------
    // repeat <N> <command ...> - run a command N times
    // ---------------------------------------------------------------
    registry.Register({"repeat", "repeat <N> <command ...>",
        "Run a command N times",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.size() < 2) return ConsoleResult::Error("Usage: repeat <N> <command ...>");

            int n = 0;
            try { n = std::stoi(args[0]); } catch (...) {
                return ConsoleResult::Error("Invalid count: " + args[0]);
            }
            if (n <= 0 || n > 10000) return ConsoleResult::Error("Count must be 1-10000");

            auto* console = ConsoleModule::Instance();
            if (!console) return ConsoleResult::Error("Console not available");

            std::string cmdLine;
            for (size_t i = 1; i < args.size(); ++i) {
                if (i > 1) cmdLine += ' ';
                cmdLine += args[i];
            }

            std::ostringstream os;
            for (int i = 0; i < n; ++i) {
                auto result = console->Execute(cmdLine);
                if (!result.text.empty() && i == n - 1)
                    os << result.text;
            }
            os << "\n--- repeated " << n << " times ---";
            return ConsoleResult::Ok(os.str());
        }, nullptr
    });

    // ---------------------------------------------------------------
    // sleep <ms> - pause for N milliseconds (useful in exec scripts)
    // ---------------------------------------------------------------
    registry.Register({"sleep", "sleep <ms>",
        "Pause for N milliseconds",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty()) return ConsoleResult::Error("Usage: sleep <ms>");
            int ms = 0;
            try { ms = std::stoi(args[0]); } catch (...) {
                return ConsoleResult::Error("Invalid duration: " + args[0]);
            }
            if (ms <= 0 || ms > 30000) return ConsoleResult::Error("Duration must be 1-30000 ms");
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            char buf[64];
            snprintf(buf, sizeof(buf), "Slept %d ms", ms);
            return ConsoleResult::Ok(buf);
        }, nullptr
    });

    // ---------------------------------------------------------------
    // stat.fps - show current FPS and frame time
    // ---------------------------------------------------------------
    registry.Register({"stat.fps", "stat.fps",
        "Show current FPS and frame time",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            auto& prof = PerformanceProfiler::GetInstance();
            double fps = prof.GetFPS();
            double ms = prof.GetFrameDuration();
            char buf[128];
            snprintf(buf, sizeof(buf), "FPS: %.1f  Frame: %.2f ms", fps, ms);
            return ConsoleResult::Ok(buf);
        }, nullptr
    });

    // ---------------------------------------------------------------
    // stat.perf - show profiler hotspots for current frame
    // ---------------------------------------------------------------
    registry.Register({"stat.perf", "stat.perf [N]",
        "Show top N profiler hotspots (default 10)",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            auto& prof = PerformanceProfiler::GetInstance();
            if (!prof.IsEnabled())
                return ConsoleResult::Ok("Profiler disabled. Use perf-enable to start.");

            size_t n = 10;
            if (!args.empty()) {
                try { n = static_cast<size_t>(std::stoi(args[0])); } catch (...) {}
            }

            auto& frame = prof.GetCurrentFrameData();
            if (frame.samples.empty())
                return ConsoleResult::Ok("No profiler samples this frame");

            // Sort by duration descending
            std::vector<std::pair<std::string, ProfileSample>> sorted(
                frame.samples.begin(), frame.samples.end());
            std::sort(sorted.begin(), sorted.end(),
                [](auto& a, auto& b) { return a.second.duration > b.second.duration; });

            std::ostringstream os;
            os << "Frame " << frame.frameNumber
               << " (" << std::fixed << std::setprecision(2)
               << frame.totalTime << " ms):\n";

            size_t count = std::min(n, sorted.size());
            for (size_t i = 0; i < count; ++i) {
                auto& [name, s] = sorted[i];
                char line[128];
                snprintf(line, sizeof(line), "  %-30s %6.2f ms  (%dx)",
                         name.c_str(), s.duration, s.callCount);
                os << line << "\n";
            }
            return ConsoleResult::Ok(os.str());
        }, nullptr
    });

    // ---------------------------------------------------------------
    // stat.memory - show memory usage summary
    // ---------------------------------------------------------------
    registry.Register({"stat.memory", "stat.memory",
        "Show memory usage summary",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            // Delegate to existing mem-stats
            auto* console = ConsoleModule::Instance();
            if (!console) return ConsoleResult::Error("Console not available");
            return console->Execute("mem-stats");
        }, nullptr
    });

    // ---------------------------------------------------------------
    // stat.scene - show scene object counts
    // ---------------------------------------------------------------
    registry.Register({"stat.scene", "stat.scene",
        "Show scene statistics",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            if (!kernel.Services().Has("script_bridge"))
                return ConsoleResult::Ok("No scene loaded (no script bridge)");
            return ConsoleResult::Ok("Scene statistics: use 'inspect Scene' for details");
        }, nullptr
    });

    // ---------------------------------------------------------------
    // stat.off - alias for perf-disable
    // ---------------------------------------------------------------
    registry.Register({"stat.off", "stat.off",
        "Disable performance stats collection",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            PerformanceProfiler::GetInstance().SetEnabled(false);
            return ConsoleResult::Ok("Stats collection disabled");
        }, nullptr
    });

    // ---------------------------------------------------------------
    // pause - freeze simulation ticking
    // ---------------------------------------------------------------
    registry.Register({"pause", "pause",
        "Pause simulation (rendering continues, ticking stops)",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            cvar_sim_pause.Set(true);
            return ConsoleResult::Ok("Simulation paused. Use 'unpause' or 'step' to continue.");
        }, nullptr
    });

    // ---------------------------------------------------------------
    // unpause - resume simulation ticking
    // ---------------------------------------------------------------
    registry.Register({"unpause", "unpause",
        "Resume simulation ticking",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            cvar_sim_pause.Set(false);
            g_simStepFrames.store(0);
            return ConsoleResult::Ok("Simulation resumed.");
        }, nullptr
    });

    // ---------------------------------------------------------------
    // step [N] - advance N frames while paused (default 1)
    // ---------------------------------------------------------------
    registry.Register({"step", "step [N]",
        "Advance N simulation frames while paused (default 1)",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            int n = 1;
            if (!args.empty()) {
                try { n = std::stoi(args[0]); } catch (...) {
                    return ConsoleResult::Error("Invalid count: " + args[0]);
                }
            }
            if (n <= 0 || n > 1000) return ConsoleResult::Error("Count must be 1-1000");

            if (!cvar_sim_pause.Get()) {
                cvar_sim_pause.Set(true);
            }
            g_simStepFrames.fetch_add(n);
            char buf[64];
            snprintf(buf, sizeof(buf), "Stepping %d frame(s)", n);
            return ConsoleResult::Ok(buf);
        }, nullptr
    });

    // ---------------------------------------------------------------
    // watch <cvar> - add a CVar to the on-screen debug overlay
    // ---------------------------------------------------------------
    registry.Register({"watch", "watch <cvar_name>",
        "Watch a CVar on the debug overlay (on-screen HUD)",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (!g_debugOverlay)
                return ConsoleResult::Error("Debug overlay not initialized");
            if (args.empty())
                return ConsoleResult::Error("Usage: watch <cvar_name>");

            auto& cvars = CVarSystem::Get();
            const std::string& name = args[0];
            auto* param = cvars.Find(name);
            if (!param)
                return ConsoleResult::NotFound("CVar not found: " + name);

            // Capture the parameter pointer for the getter
            g_debugOverlay->Add(name, [param]() -> std::string {
                switch (param->type) {
                    case CVarType::Int:    return std::to_string(param->intVal);
                    case CVarType::Float: {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%.3f", param->floatVal);
                        return buf;
                    }
                    case CVarType::Bool:   return param->boolVal ? "true" : "false";
                    case CVarType::String: return param->strVal;
                    default:               return "?";
                }
            });
            return ConsoleResult::Ok("Watching: " + name);
        },
        // Tab-complete CVar names
        [](KoiloKernel&, const std::vector<std::string>&, const std::string& partial) -> std::vector<std::string> {
            std::vector<std::string> matches;
            CVarSystem::Get().ForEach([&](const CVarParameter& p) {
                if (p.name.find(partial) == 0) matches.push_back(p.name);
            });
            return matches;
        }
    });

    // ---------------------------------------------------------------
    // unwatch <name|*> - remove a watch from the overlay
    // ---------------------------------------------------------------
    registry.Register({"unwatch", "unwatch <name|*>",
        "Remove a watch from the debug overlay (* = clear all)",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (!g_debugOverlay)
                return ConsoleResult::Error("Debug overlay not initialized");
            if (args.empty())
                return ConsoleResult::Error("Usage: unwatch <name|*>");

            if (args[0] == "*") {
                size_t count = g_debugOverlay->Count();
                g_debugOverlay->Clear();
                return ConsoleResult::Ok("Cleared " + std::to_string(count) + " watch(es)");
            }
            if (g_debugOverlay->Remove(args[0]))
                return ConsoleResult::Ok("Removed: " + args[0]);
            return ConsoleResult::NotFound("Not watching: " + args[0]);
        },
        // Tab-complete watched names
        [](KoiloKernel&, const std::vector<std::string>&, const std::string& partial) -> std::vector<std::string> {
            std::vector<std::string> matches;
            if (!g_debugOverlay) return matches;
            for (auto& e : g_debugOverlay->Entries()) {
                if (e.name.find(partial) == 0) matches.push_back(e.name);
            }
            if (std::string("*").find(partial) == 0) matches.push_back("*");
            return matches;
        }
    });

    // ---------------------------------------------------------------
    // watches - list all active watches
    // ---------------------------------------------------------------
    registry.Register({"watches", "watches",
        "List all active debug overlay watches",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            if (!g_debugOverlay)
                return ConsoleResult::Error("Debug overlay not initialized");
            auto entries = g_debugOverlay->Entries();
            if (entries.empty())
                return ConsoleResult::Ok("No active watches. Use 'watch <cvar>' to add one.");
            std::ostringstream os;
            os << "Active watches (" << entries.size() << "):\n";
            for (auto& e : entries) {
                std::string val;
                try { val = e.getter(); } catch (...) { val = "<error>"; }
                os << "  " << e.name << " = " << val << "\n";
            }
            return ConsoleResult::Ok(os.str());
        }, nullptr
    });
}

} // namespace koilo
