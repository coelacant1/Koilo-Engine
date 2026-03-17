#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/systems/profiling/memoryprofiler.hpp>
#include <koilo/systems/profiling/performanceprofiler.hpp>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace koilo {

void RegisterProfilingCommands(CommandRegistry& registry) {
    // -- perf --
    registry.Register({"perf", "perf [frame|fps|hotspots|reset]", "Performance profiler stats",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            auto& prof = PerformanceProfiler::GetInstance();
            if (!prof.IsEnabled()) {
                return ConsoleResult::Ok("Performance profiler is disabled. Use 'perf-enable' to enable.");
            }

            std::string sub = args.empty() ? "frame" : args[0];

            if (sub == "fps") {
                double fps = prof.GetFPS();
                double frameMs = prof.GetFrameDuration();
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(1)
                   << "FPS: " << fps << "  Frame: " << frameMs << " ms";
                std::ostringstream js;
                js << "{\"fps\":" << fps << ",\"frame_ms\":" << frameMs << "}";
                return ConsoleResult::Ok(ss.str(), js.str());
            }

            if (sub == "frame") {
                auto& frame = prof.GetCurrentFrameData();
                std::ostringstream ss;
                ss << "Frame #" << frame.frameNumber
                   << "  Total: " << std::fixed << std::setprecision(2)
                   << frame.totalTime << " ms\n";

                // Sort samples by duration
                std::vector<std::pair<std::string, ProfileSample>> sorted(
                    frame.samples.begin(), frame.samples.end());
                std::sort(sorted.begin(), sorted.end(),
                    [](auto& a, auto& b) { return a.second.duration > b.second.duration; });

                for (auto& [name, sample] : sorted) {
                    ss << "  " << std::left << std::setw(24) << name
                       << std::right << std::setw(8) << std::fixed << std::setprecision(3)
                       << sample.duration << " ms"
                       << "  x" << sample.callCount << "\n";
                }
                return ConsoleResult::Ok(ss.str());
            }

            if (sub == "hotspots") {
                int n = 10;
                if (args.size() > 1) {
                    try { n = std::stoi(args[1]); } catch (...) {}
                }
                auto& samples = prof.GetAllSamples();
                std::vector<std::pair<std::string, ProfileSample>> sorted(
                    samples.begin(), samples.end());
                std::sort(sorted.begin(), sorted.end(),
                    [](auto& a, auto& b) { return a.second.duration > b.second.duration; });

                std::ostringstream ss;
                ss << "Top " << n << " hotspots:\n";
                int count = 0;
                for (auto& [name, sample] : sorted) {
                    if (count++ >= n) break;
                    ss << "  " << std::left << std::setw(24) << name
                       << std::fixed << std::setprecision(3)
                       << "avg=" << prof.GetAverageDuration(name) << " ms"
                       << "  min=" << sample.minDuration
                       << "  max=" << sample.maxDuration
                       << "  calls=" << sample.callCount << "\n";
                }
                return ConsoleResult::Ok(ss.str());
            }

            if (sub == "reset") {
                prof.Clear();
                return ConsoleResult::Ok("Performance profiler reset.");
            }

            return ConsoleResult::Error("Unknown perf subcommand: " + sub);
        }, [](KoiloKernel&, const std::vector<std::string>&, const std::string& partial) -> std::vector<std::string> {
            std::vector<std::string> opts = {"frame", "fps", "hotspots", "reset"};
            if (partial.empty()) return opts;
            std::vector<std::string> m;
            for (auto& o : opts) if (o.find(partial) == 0) m.push_back(o);
            return m;
        }
    });

    // -- perf-enable / perf-disable --
    registry.Register({"perf-enable", "perf-enable", "Enable performance profiler",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            PerformanceProfiler::GetInstance().SetEnabled(true);
            return ConsoleResult::Ok("Performance profiler enabled.");
        }, nullptr
    });
    registry.Register({"perf-disable", "perf-disable", "Disable performance profiler",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            PerformanceProfiler::GetInstance().SetEnabled(false);
            return ConsoleResult::Ok("Performance profiler disabled.");
        }, nullptr
    });

    // -- mem-report --
    registry.Register({"mem-report", "mem-report", "Full memory profiler report",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            auto& mp = MemoryProfiler::GetInstance();
            if (!mp.IsEnabled()) {
                return ConsoleResult::Ok("Memory profiler is disabled. Use 'mem-enable' to enable.");
            }
            return ConsoleResult::Ok(mp.GetReportString());
        }, nullptr
    });

    // -- mem-stats --
    registry.Register({"mem-stats", "mem-stats", "Memory profiler statistics summary",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            auto& mp = MemoryProfiler::GetInstance();
            auto& stats = mp.GetStats();
            std::ostringstream ss;
            ss << "Memory Stats:\n"
               << "  Total allocated: " << MemoryProfiler::FormatBytes(stats.totalAllocated) << "\n"
               << "  Total freed:     " << MemoryProfiler::FormatBytes(stats.totalFreed) << "\n"
               << "  Current usage:   " << MemoryProfiler::FormatBytes(stats.currentUsage) << "\n"
               << "  Peak usage:      " << MemoryProfiler::FormatBytes(stats.peakUsage) << "\n"
               << "  Active allocs:   " << stats.activeAllocations << "\n";

            std::ostringstream js;
            js << "{\"total_allocated\":" << stats.totalAllocated
               << ",\"total_freed\":" << stats.totalFreed
               << ",\"current_usage\":" << stats.currentUsage
               << ",\"peak_usage\":" << stats.peakUsage
               << ",\"active_allocations\":" << stats.activeAllocations << "}";
            return ConsoleResult::Ok(ss.str(), js.str());
        }, nullptr
    });

    // -- mem-leaks --
    registry.Register({"mem-leaks", "mem-leaks", "Show potential memory leaks",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            auto& mp = MemoryProfiler::GetInstance();
            std::ostringstream ss;
            mp.PrintLeaks(); // prints to stderr
            auto& allocs = mp.GetAllocations();
            if (allocs.empty()) {
                ss << "No active allocations (no leaks detected).";
            } else {
                ss << "Active allocations (" << allocs.size() << "):\n";
                int shown = 0;
                for (auto& [addr, alloc] : allocs) {
                    if (shown++ >= 50) {
                        ss << "  ... and " << (allocs.size() - 50) << " more\n";
                        break;
                    }
                    ss << "  " << addr << "  "
                       << MemoryProfiler::FormatBytes(alloc.size)
                       << "  [" << alloc.tag << "]\n";
                }
            }
            return ConsoleResult::Ok(ss.str());
        }, nullptr
    });

    // -- mem-tags --
    registry.Register({"mem-tags", "mem-tags", "Show memory usage by tag",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            auto& mp = MemoryProfiler::GetInstance();
            auto& tags = mp.GetUsageByTagMap();
            if (tags.empty()) return ConsoleResult::Ok("No tagged allocations.");

            // Sort by usage descending
            std::vector<std::pair<std::string, size_t>> sorted(tags.begin(), tags.end());
            std::sort(sorted.begin(), sorted.end(),
                [](auto& a, auto& b) { return a.second > b.second; });

            std::ostringstream ss;
            ss << "Memory by tag:\n";
            for (auto& [tag, bytes] : sorted) {
                ss << "  " << std::left << std::setw(20) << tag
                   << std::right << std::setw(12) << MemoryProfiler::FormatBytes(bytes) << "\n";
            }
            return ConsoleResult::Ok(ss.str());
        }, nullptr
    });

    // -- mem-enable / mem-disable --
    registry.Register({"mem-enable", "mem-enable", "Enable memory profiler",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            MemoryProfiler::GetInstance().SetEnabled(true);
            return ConsoleResult::Ok("Memory profiler enabled.");
        }, nullptr
    });
    registry.Register({"mem-disable", "mem-disable", "Disable memory profiler",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            MemoryProfiler::GetInstance().SetEnabled(false);
            return ConsoleResult::Ok("Memory profiler disabled.");
        }, nullptr
    });
}

} // namespace koilo
