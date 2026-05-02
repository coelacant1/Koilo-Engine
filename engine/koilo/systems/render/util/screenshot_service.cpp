// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/render/util/screenshot_service.hpp>
#include <koilo/systems/render/util/bmp_writer.hpp>
#include <koilo/systems/render/rhi/rhi_device.hpp>

namespace koilo::render::util {

ScreenshotService& ScreenshotService::Get() {
    static ScreenshotService instance;
    return instance;
}

bool ScreenshotService::Request(const std::string& path) {
    auto cur = status_.load(std::memory_order_acquire);
    if (cur == ScreenshotStatus::Pending) return false;

    std::lock_guard<std::mutex> lock(mu_);
    path_ = path;
    message_.clear();
    status_.store(ScreenshotStatus::Pending, std::memory_order_release);
    return true;
}

std::string ScreenshotService::Message() const {
    std::lock_guard<std::mutex> lock(mu_);
    return message_;
}

void ScreenshotService::Reset() {
    std::lock_guard<std::mutex> lock(mu_);
    path_.clear();
    message_.clear();
    status_.store(ScreenshotStatus::Idle, std::memory_order_release);
}

void ScreenshotService::ServicePending(rhi::IRHIDevice* device) {
    if (status_.load(std::memory_order_acquire) != ScreenshotStatus::Pending)
        return;

    std::string path;
    {
        std::lock_guard<std::mutex> lock(mu_);
        path = path_;
    }

    auto fail = [&](const std::string& msg) {
        std::lock_guard<std::mutex> lock(mu_);
        message_ = msg;
        status_.store(ScreenshotStatus::Failed, std::memory_order_release);
    };
    auto done = [&](const std::string& msg) {
        std::lock_guard<std::mutex> lock(mu_);
        message_ = msg;
        status_.store(ScreenshotStatus::Done, std::memory_order_release);
    };

    if (!device)      { fail("no rhi device"); return; }
    if (path.empty()) { fail("empty path"); return; }

    const uint8_t* pixels = device->GetSwapchainPixels();
    if (!pixels) {
        fail("backend has no synchronous swapchain readback "
             "(currently only Software RHI is supported; "
             "run with --software to capture)");
        return;
    }

    uint32_t w = 0, h = 0;
    device->GetSwapchainSize(w, h);
    if (w == 0 || h == 0) { fail("zero-size swapchain"); return; }

    if (!WriteBMP24(path, pixels, w, h, /*topDown=*/true)) {
        fail("failed to write file: " + path);
        return;
    }

    done(path);
}

} // namespace koilo::render::util
