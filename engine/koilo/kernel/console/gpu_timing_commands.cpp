// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file gpu_timing_commands.cpp
 * @brief Console commands for GPU timing queries and frame timeline capture.
 *
 * Commands:
 *   stat.gpu              -- Show per-pass GPU timing breakdown
 *   stat.gpu.toggle       -- Enable/disable GPU timing collection
 *   profile.capture [N]   -- Capture N frames (default 60) to .kprof JSON
 */
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <koilo/systems/profiling/gpu_timing.hpp>
#include <koilo/systems/profiling/performanceprofiler.hpp>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vector>

namespace koilo {

// -- Frame timeline capture state -------------------------------------------

namespace {

struct CapturedFrame {
    int                        frameNumber;
    double                     cpuFrameMs;
    std::vector<GPUPassTiming> gpuPasses;
    double                     gpuTotalMs;
};

struct CaptureState {
    bool                       active        = false;
    int                        remainingFrames = 0;
    int                        startFrame    = 0;
    std::string                outputPath;
    std::vector<CapturedFrame> frames;
};

static CaptureState g_capture;

/// Write captured frames to a chrome://tracing compatible JSON file.
static bool WriteChromeTracing(const std::string& path,
                               const std::vector<CapturedFrame>& frames) {
    std::ofstream out(path);
    if (!out.is_open()) return false;

    out << "[\n";
    bool first = true;

    for (const auto& f : frames) {
        // CPU frame event
        double frameStartUs = f.frameNumber * f.cpuFrameMs * 1000.0;

        if (!first) out << ",\n";
        first = false;
        out << "  {\"name\":\"Frame " << f.frameNumber
            << "\",\"cat\":\"cpu\",\"ph\":\"X\""
            << ",\"ts\":" << std::fixed << std::setprecision(1) << frameStartUs
            << ",\"dur\":" << std::fixed << std::setprecision(1) << (f.cpuFrameMs * 1000.0)
            << ",\"pid\":1,\"tid\":0}";

        // GPU pass events
        double gpuOffset = 0.0;
        for (const auto& pass : f.gpuPasses) {
            out << ",\n  {\"name\":\"" << pass.name
                << "\",\"cat\":\"gpu\",\"ph\":\"X\""
                << ",\"ts\":" << std::fixed << std::setprecision(1)
                << (frameStartUs + gpuOffset * 1000.0)
                << ",\"dur\":" << std::fixed << std::setprecision(1)
                << (pass.ms * 1000.0)
                << ",\"pid\":1,\"tid\":1}";
            gpuOffset += pass.ms;
        }
    }

    out << "\n]\n";
    return true;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Console commands
// ---------------------------------------------------------------------------

void RegisterGPUTimingCommands(CommandRegistry& registry) {

    // stat.gpu - show per-pass GPU timing breakdown
    registry.Register({"stat.gpu", "stat.gpu",
        "Show per-pass GPU timing breakdown",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto* timing = kernel.Services().Get<GPUTimingManager>("gpu_timing");
            if (!timing)
                return ConsoleResult::Error("GPU timing not available (no RHI or no timestamp support)");
            if (!timing->IsEnabled())
                return ConsoleResult::Error("GPU timing disabled. Use stat.gpu.toggle to enable.");

            const auto& passes = timing->GetPassTimings();
            if (passes.empty())
                return ConsoleResult::Ok("No GPU timing data yet (wait one frame)");

            std::ostringstream ss;
            ss << "GPU Pass Timing:\n";
            ss << std::fixed << std::setprecision(2);
            for (const auto& p : passes) {
                ss << "  " << std::setw(20) << std::left << p.name
                   << std::setw(8) << std::right << p.ms << " ms\n";
            }
            ss << "  " << std::string(28, '-') << "\n";
            ss << "  " << std::setw(20) << std::left << "Total"
               << std::setw(8) << std::right << timing->GetTotalGPUTimeMs() << " ms";

            return ConsoleResult::Ok(ss.str());
        }});

    // stat.gpu.toggle - enable/disable GPU timing
    registry.Register({"stat.gpu.toggle", "stat.gpu.toggle",
        "Enable or disable GPU timing collection",
        [](KoiloKernel& kernel, const std::vector<std::string>&) -> ConsoleResult {
            auto* timing = kernel.Services().Get<GPUTimingManager>("gpu_timing");
            if (!timing)
                return ConsoleResult::Error("GPU timing not available");

            bool newState = !timing->IsEnabled();
            timing->SetEnabled(newState);
            return ConsoleResult::Ok(std::string("GPU timing ") +
                                     (newState ? "enabled" : "disabled"));
        }});

    // profile.capture - capture N frames of CPU+GPU timing to .kprof file
    registry.Register({"profile.capture", "profile.capture [frames] [path]",
        "Capture N frames of timing data to chrome://tracing JSON",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (g_capture.active)
                return ConsoleResult::Error("Capture already in progress ("
                    + std::to_string(g_capture.remainingFrames) + " frames remaining)");

            int frames = 60;
            std::string path = "profile_capture.kprof";

            if (!args.empty()) {
                try { frames = std::stoi(args[0]); }
                catch (...) { return ConsoleResult::Error("Invalid frame count: " + args[0]); }
                if (frames < 1 || frames > 10000)
                    return ConsoleResult::Error("Frame count must be 1-10000");
            }
            if (args.size() >= 2) {
                path = args[1];
            }

            // Ensure GPU timing is enabled
            auto* timing = kernel.Services().Get<GPUTimingManager>("gpu_timing");
            if (timing && !timing->IsEnabled()) {
                timing->SetEnabled(true);
            }

            g_capture.active         = true;
            g_capture.remainingFrames = frames;
            g_capture.startFrame     = 0;
            g_capture.outputPath     = path;
            g_capture.frames.clear();
            g_capture.frames.reserve(static_cast<size_t>(frames));

            return ConsoleResult::Ok("Capturing " + std::to_string(frames)
                + " frames to " + path + "...");
        }});

    // profile.status - check capture progress
    registry.Register({"profile.status", "profile.status",
        "Show profile capture status",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            if (!g_capture.active)
                return ConsoleResult::Ok("No capture in progress");

            int captured = static_cast<int>(g_capture.frames.size());
            int total    = captured + g_capture.remainingFrames;
            return ConsoleResult::Ok("Capturing: " + std::to_string(captured)
                + "/" + std::to_string(total) + " frames");
        }});
}

// ---------------------------------------------------------------------------
// Frame capture tick - call once per frame from the host loop
// ---------------------------------------------------------------------------

void TickProfileCapture(KoiloKernel& kernel, int frameNumber) {
    if (!g_capture.active) return;

    auto& prof = PerformanceProfiler::GetInstance();
    auto* timing = kernel.Services().Get<GPUTimingManager>("gpu_timing");

    CapturedFrame cf;
    cf.frameNumber = frameNumber;
    cf.cpuFrameMs  = prof.GetFrameDuration();
    cf.gpuTotalMs  = 0.0;

    if (timing && timing->IsEnabled()) {
        cf.gpuPasses  = timing->GetPassTimings();
        cf.gpuTotalMs = timing->GetTotalGPUTimeMs();
    }

    g_capture.frames.push_back(std::move(cf));
    g_capture.remainingFrames--;

    if (g_capture.remainingFrames <= 0) {
        g_capture.active = false;

        // Write the file
        if (WriteChromeTracing(g_capture.outputPath, g_capture.frames)) {
            KL_LOG("Profile", "Captured %zu frames to %s",
                   g_capture.frames.size(), g_capture.outputPath.c_str());
        } else {
            KL_ERR("Profile", "Failed to write capture to %s",
                   g_capture.outputPath.c_str());
        }
        g_capture.frames.clear();
    }
}

} // namespace koilo
