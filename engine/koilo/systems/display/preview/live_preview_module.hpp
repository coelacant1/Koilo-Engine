// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file live_preview_module.hpp
 * @brief Kernel module for MJPEG live preview over HTTP.
 *
 * Serves the engine's swapchain as a BMP MJPEG stream over HTTP.
 * Open http://<host>:<port>/ in any browser to view the output.
 *
 * Architecture:
 *   - A background thread runs a minimal HTTP server.
 *   - GET /        -> embedded HTML viewer page.
 *   - GET /stream  -> multipart/x-mixed-replace BMP stream.
 *   - Tick() reads the swapchain, encodes a BMP, and pushes it
 *     to all connected stream clients via non-blocking send().
 *
 * No external dependencies. Uses POSIX sockets (Linux/macOS).
 * Conditionally compiled behind KL_HAVE_LIVE_PREVIEW.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#pragma once

#ifdef KL_HAVE_LIVE_PREVIEW

#include <koilo/kernel/module.hpp>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>

namespace koilo {

/**
 * @class LivePreviewModule
 * @brief Kernel module that streams swapchain frames over HTTP as MJPEG.
 *
 * Lifecycle:
 *   1. Init: Opens TCP listener, starts server thread.
 *   2. Tick: Captures swapchain, encodes BMP, sends to stream clients.
 *   3. Shutdown: Stops server thread, closes all sockets.
 *
 * Registered as "koilo.live_preview" with the kernel module system.
 */
class LivePreviewModule {
public:
    LivePreviewModule();
    ~LivePreviewModule();

    LivePreviewModule(const LivePreviewModule&) = delete;
    LivePreviewModule& operator=(const LivePreviewModule&) = delete;

    /**
     * @brief Get the module descriptor for kernel registration.
     * @return Reference to the static ModuleDesc.
     */
    static const ModuleDesc& GetModuleDesc();

    /**
     * @brief Get the singleton instance (valid after Init).
     * @return Pointer to the module instance, or nullptr.
     */
    static LivePreviewModule* Instance() { return instance_.get(); }

    /**
     * @brief Get the number of connected stream clients.
     * @return Client count.
     */
    uint32_t GetClientCount() const;

    /**
     * @brief Get the total frames streamed since Init.
     * @return Frame count.
     */
    uint64_t GetFrameCount() const { return framesStreamed_; }

    /**
     * @brief Encode an RGBA8 image as a 24-bit BMP (top-down).
     * @param rgba Source pixels (4 bytes per pixel, RGBA order).
     * @param width Image width in pixels.
     * @param height Image height in pixels.
     * @param out Output buffer (resized to hold the BMP).
     *
     * Public for testing. No external dependencies.
     */
    static void EncodeBMP(const uint8_t* rgba, uint32_t width,
                          uint32_t height, std::vector<uint8_t>& out);

    /**
     * @brief Parse a minimal HTTP request line.
     * @param data Raw request data.
     * @param len Length of data.
     * @param outPath Parsed request path (e.g. "/" or "/stream").
     * @return True if a valid GET request was parsed.
     *
     * Public for testing.
     */
    static bool ParseHTTPRequest(const char* data, size_t len,
                                 std::string& outPath);

private:
    static bool Init(KoiloKernel& kernel);
    static void Tick(float dt);
    static void OnMessage(const Message& msg);
    static void Shutdown();

    void RegisterCommands();
    void ServerThreadFunc();
    void HandleNewConnection(int fd);
    void SendHTMLPage(int fd);
    void SendStreamHeader(int fd);
    void BroadcastFrame(const std::vector<uint8_t>& frame);
    static bool SendAll(int fd, const void* data, size_t len);

    static std::unique_ptr<LivePreviewModule> instance_;

    KoiloKernel*             kernel_;
    int                      listenFd_;
    std::thread              serverThread_;
    std::atomic<bool>        running_;

    mutable std::mutex       clientsMutex_;
    std::vector<int>         streamClients_;

    std::mutex               frameMutex_;       ///< Protects bmpBuffer_ + frameReady_
    std::vector<uint8_t>     bmpBuffer_;
    bool                     frameReady_;       ///< New frame staged for server thread
    std::atomic<uint32_t>    clientCount_;      ///< Fast client count for Tick() skip
    uint64_t                 framesStreamed_;

    using Clock = std::chrono::steady_clock;
    Clock::time_point        lastFrameTime_;
};

} // namespace koilo

#endif // KL_HAVE_LIVE_PREVIEW
