// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
/**
 * @file screenshot_service.hpp
 * @brief Cross-thread screenshot request bus.
 *
 * The console runs on the network/REPL thread and cannot safely touch the
 * GPU device directly.  This singleton is a request bus: the console
 * posts a screenshot request with Request(), the host loop services it
 * after each frame's Present, and the requester polls Result() until
 * status != Pending.
 *
 * Today only Software RHI is supported (synchronous read from the just-
 * rendered swapchain buffer).  Vulkan support is queued for a follow-up
 * once the readback path lands; until then a request on a Vulkan device
 * completes with Failed + a clear message.
 */

#include <atomic>
#include <mutex>
#include <string>

namespace koilo::rhi { class IRHIDevice; }

namespace koilo::render::util {

enum class ScreenshotStatus : uint8_t {
    Idle,
    Pending,
    Done,
    Failed
};

class ScreenshotService {
public:
    static ScreenshotService& Get();

    /// Post a screenshot request.  Returns false if a request is already
    /// pending (caller should try again later).  The status transitions
    /// to Pending and the host loop will pick it up next frame.
    bool Request(const std::string& path);

    /// Read the current request status.  Caller polls this from the
    /// console thread after Request() to wait for completion.
    ScreenshotStatus Status() const { return status_.load(std::memory_order_acquire); }

    /// Last result message (file path on success, error string on
    /// failure).  Stable until the next Request().
    std::string Message() const;

    /// Reset to Idle.  Called automatically after Request() completes
    /// and the requester reads the result; callers may also call this
    /// explicitly to discard a pending result.
    void Reset();

    /// Host loop hook -- invoked after each Present.  Captures and
    /// writes the BMP if a request is pending.  Safe to call every
    /// frame; cheap no-op when Idle.
    ///
    /// The device's swapchain pixel buffer is read via
    /// IRHIDevice::GetSwapchainPixels (Software RHI only today).
    void ServicePending(rhi::IRHIDevice* device);

private:
    ScreenshotService() = default;

    mutable std::mutex          mu_;
    std::string                 path_;
    std::string                 message_;
    std::atomic<ScreenshotStatus> status_{ScreenshotStatus::Idle};
};

} // namespace koilo::render::util
