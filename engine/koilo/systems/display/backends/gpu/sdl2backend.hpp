// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file sdl2backend.hpp
 * @brief SDL2 software rendering display backend.
 *
 * This backend uses SDL2's software renderer to display frames without
 * requiring OpenGL or any GPU acceleration. Perfect for pure software
 * rendering pipelines.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <koilo/systems/display/idisplaybackend.hpp>
#include <string>
#include <koilo/registry/reflect_macros.hpp>

// Forward declarations to avoid SDL2 header dependency
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace koilo {

/**
 * @class SDL2Backend
 * @brief Software rendering display backend using SDL2.
 *
 * This backend creates a native window and renders frames using SDL2's
 * software renderer (no GPU/OpenGL required).
 *
 * Features:
 * - Pure software rendering
 * - No OpenGL/Vulkan/DirectX dependency
 * - Cross-platform (Linux, Windows, macOS)
 * - VSync support
 * - Resizable window
 * - Multiple scaling modes
 * - Hardware texture upload (if available)
 *
 * Usage:
 * @code
 * auto backend = make_unique<SDL2Backend>(800, 600, "My Game");
 * displayMgr.AddDisplay(move(backend));
 * // Frames are displayed in a software-rendered window
 * @endcode
 */
class SDL2Backend : public IDisplayBackend {
public:
    /**
     * @enum ScalingMode
     * @brief How to scale framebuffer to window.
     */
    enum class ScalingMode {
        Stretch,        ///< Stretch to fill window (may distort)
        Fit,            ///< Fit inside window (maintain aspect ratio)
        Fill,           ///< Fill window (may crop)
        None,           ///< No scaling (1:1 pixel mapping)
    };

    /**
     * @brief Constructor.
     * @param width Window width in pixels.
     * @param height Window height in pixels.
     * @param title Window title.
     * @param fullscreen Start in fullscreen mode.
     * @param resizable Allow window resizing.
     */
    SDL2Backend(uint32_t width, uint32_t height,
                const std::string& title = "KoiloEngine",
                bool fullscreen = false,
                bool resizable = true);

    /**
     * @brief Destructor.
     */
    ~SDL2Backend() override;

    // === IDisplayBackend Interface ===

    bool Initialize() override;
    void Shutdown() override;
    bool IsInitialized() const override;

    DisplayInfo GetInfo() const override;
    bool HasCapability(DisplayCapability cap) const override;

    bool Present(const Framebuffer& fb) override;
    void WaitVSync() override;
    bool Clear() override;

    bool SetRefreshRate(uint32_t hz) override;
    bool SetOrientation(Orientation orient) override;
    bool SetBrightness(uint8_t brightness) override;
    bool SetVSyncEnabled(bool enabled) override;

    // === SDL2 Specific ===

    /**
     * @brief Set scaling mode.
     */
    void SetScalingMode(ScalingMode mode) { scalingMode_ = mode; }

    /**
     * @brief Get scaling mode.
     */
    ScalingMode GetScalingMode() const { return scalingMode_; }

    /**
     * @brief Set window title.
     */
    void SetTitle(const std::string& title);

    /**
     * @brief Toggle fullscreen mode.
     */
    bool SetFullscreen(bool fullscreen);

    /**
     * @brief Check if window is open and valid.
     */
    bool IsWindowOpen() const;

    /**
     * @brief Process window events (call once per frame).
     * @return False if window should close.
     */
    bool ProcessEvents();

    /**
     * @brief Get actual window size (may differ from framebuffer).
     */
    void GetWindowSize(uint32_t& width, uint32_t& height) const;

private:
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;

    uint32_t windowWidth_;
    uint32_t windowHeight_;
    uint32_t framebufferWidth_;
    uint32_t framebufferHeight_;

    std::string title_;
    bool fullscreen_;
    bool resizable_;
    bool initialized_;
    bool vsyncEnabled_;
    bool shouldClose_;

    ScalingMode scalingMode_;

    DisplayInfo info_;

    // Helper methods
    bool InitSDL();
    void CalculateViewport(uint32_t fbWidth, uint32_t fbHeight,
                          int& x, int& y, int& width, int& height);
    uint32_t ConvertPixelFormat(PixelFormat format);

    KL_BEGIN_FIELDS(SDL2Backend)
        KL_FIELD(SDL2Backend, windowWidth_, "Window width", 0, 65535),
        KL_FIELD(SDL2Backend, windowHeight_, "Window height", 0, 65535),
        KL_FIELD(SDL2Backend, framebufferWidth_, "Framebuffer width", 0, 65535),
        KL_FIELD(SDL2Backend, framebufferHeight_, "Framebuffer height", 0, 65535),
        KL_FIELD(SDL2Backend, title_, "Title", 0, 0),
        KL_FIELD(SDL2Backend, fullscreen_, "Fullscreen", 0, 1),
        KL_FIELD(SDL2Backend, resizable_, "Resizable", 0, 1),
        KL_FIELD(SDL2Backend, initialized_, "Initialized", 0, 1),
        KL_FIELD(SDL2Backend, vsyncEnabled_, "Vsync enabled", 0, 1),
        KL_FIELD(SDL2Backend, shouldClose_, "Should close", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(SDL2Backend)
        KL_METHOD_AUTO(SDL2Backend, Initialize, "Initialize"),
        KL_METHOD_AUTO(SDL2Backend, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(SDL2Backend, IsInitialized, "Is initialized"),
        KL_METHOD_AUTO(SDL2Backend, GetInfo, "Get info"),
        KL_METHOD_AUTO(SDL2Backend, Present, "Present"),
        KL_METHOD_AUTO(SDL2Backend, Clear, "Clear"),
        KL_METHOD_AUTO(SDL2Backend, SetTitle, "Set title"),
        KL_METHOD_AUTO(SDL2Backend, SetFullscreen, "Set fullscreen"),
        KL_METHOD_AUTO(SDL2Backend, IsWindowOpen, "Is window open"),
        KL_METHOD_AUTO(SDL2Backend, ProcessEvents, "Process events")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SDL2Backend)
        KL_CTOR(SDL2Backend, uint32_t, uint32_t, const std::string&, bool, bool)
    KL_END_DESCRIBE(SDL2Backend)
};

} // namespace koilo
