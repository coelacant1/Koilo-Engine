// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file screenshot_commands.cpp
 * @brief Console command for capturing the rendered swapchain to a file.
 *
 * Provides a single command:
 *
 *   screenshot <path>
 *
 * Posts a screenshot request to the global ScreenshotService and waits
 * (up to ~2 s) for the host loop to service it.  Today only the Software
 * RHI backend exposes a synchronous swapchain readback; on Vulkan the
 * command returns a clear error pointing at --software for capture.
 */

#include "console_commands.hpp"
#include <koilo/systems/render/util/screenshot_service.hpp>
#include <chrono>
#include <thread>

namespace koilo {

void RegisterScreenshotCommands(CommandRegistry& registry) {
    registry.Register({
        "screenshot", "screenshot <path>",
        "Capture the rendered swapchain to a 24-bit BMP file. "
        "Software RHI only (--software) until Vulkan readback lands.",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty())
                return ConsoleResult::Error("Usage: screenshot <path>");

            auto& svc = render::util::ScreenshotService::Get();
            if (!svc.Request(args[0]))
                return ConsoleResult::Error(
                    "another screenshot is already pending; "
                    "wait and retry");

            // Poll for completion. Total wait cap ~2 s; at 60 fps this is
            // ~120 frames -- generous head-room for stalled loops.
            const auto deadline = std::chrono::steady_clock::now()
                                + std::chrono::milliseconds(2000);
            while (svc.Status() == render::util::ScreenshotStatus::Pending) {
                if (std::chrono::steady_clock::now() > deadline) {
                    svc.Reset();
                    return ConsoleResult::Error(
                        "screenshot timed out -- is the render loop running?");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }

            std::string msg = svc.Message();
            auto status = svc.Status();
            svc.Reset();

            if (status == render::util::ScreenshotStatus::Done)
                return ConsoleResult::Ok("Saved screenshot: " + msg);
            return ConsoleResult::Error("Screenshot failed: " + msg);
        },
        nullptr
    });
}

} // namespace koilo
