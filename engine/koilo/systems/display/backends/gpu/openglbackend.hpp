// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file openglbackend.hpp
 * @brief OpenGL display backend with GPU acceleration.
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
typedef void* SDL_GLContext;

namespace koilo {

/**
 * @class OpenGLBackend
 * @brief GPU-accelerated display backend using OpenGL and SDL2.
 *
 * This backend creates a native window and renders frames using OpenGL.
 * Features:
 * - Hardware-accelerated rendering
 * - VSync support
 * - Resizable window
 * - Multiple scaling modes
 * - Efficient texture upload
 *
 * Usage:
 * @code
 * auto backend = make_unique<OpenGLBackend>(1920, 1080, "KoiloEngine");
 * displayMgr.AddDisplay(move(backend));
 * // Frames are rendered to GPU-accelerated window
 * @endcode
 */
class OpenGLBackend : public IDisplayBackend {
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
    OpenGLBackend(uint32_t width, uint32_t height,
                  const std::string& title = "KoiloEngine",
                  bool fullscreen = false,
                  bool resizable = true);
    
    /**
     * @brief Destructor.
     */
    ~OpenGLBackend() override;
    
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
    
    // === OpenGL Specific ===

    /** @brief Swap buffers only (no texture upload). Use with BlitToScreen(). */
    void SwapOnly();
    /** Upload texture + draw quad without swapping. Call SwapOnly() later. */
    bool PresentNoSwap(const Framebuffer& fb);
    
    /**
     * @brief Set texture filtering to nearest-neighbor (sharp pixels) or bilinear (smooth).
     * Call after Initialize(). Use nearest for pixel-art / low-res upscaling.
     */
    void SetNearestFiltering(bool nearest);

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
    SDL_GLContext glContext_;
    
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
    
    // OpenGL resources
    unsigned int texture_;
    unsigned int vao_;
    unsigned int vbo_;
    unsigned int shaderProgram_;
    uint32_t texWidth_ = 0;
    uint32_t texHeight_ = 0;
    
    DisplayInfo info_;
    
    // Helper methods
    bool InitSDL();
    bool InitOpenGL();
    bool CreateShader();
    void RenderQuad();
    void CalculateViewport(uint32_t fbWidth, uint32_t fbHeight,
                          int& x, int& y, int& width, int& height);

    KL_BEGIN_FIELDS(OpenGLBackend)
        KL_FIELD(OpenGLBackend, windowWidth_, "Window width", 0, 65535),
        KL_FIELD(OpenGLBackend, windowHeight_, "Window height", 0, 65535),
        KL_FIELD(OpenGLBackend, framebufferWidth_, "Framebuffer width", 0, 65535),
        KL_FIELD(OpenGLBackend, framebufferHeight_, "Framebuffer height", 0, 65535),
        KL_FIELD(OpenGLBackend, title_, "Title", 0, 0),
        KL_FIELD(OpenGLBackend, fullscreen_, "Fullscreen", 0, 1),
        KL_FIELD(OpenGLBackend, resizable_, "Resizable", 0, 1),
        KL_FIELD(OpenGLBackend, initialized_, "Initialized", 0, 1),
        KL_FIELD(OpenGLBackend, vsyncEnabled_, "Vsync enabled", 0, 1),
        KL_FIELD(OpenGLBackend, shouldClose_, "Should close", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(OpenGLBackend)
        KL_METHOD_AUTO(OpenGLBackend, Initialize, "Initialize"),
        KL_METHOD_AUTO(OpenGLBackend, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(OpenGLBackend, IsInitialized, "Is initialized"),
        KL_METHOD_AUTO(OpenGLBackend, GetInfo, "Get info"),
        KL_METHOD_AUTO(OpenGLBackend, Present, "Present"),
        KL_METHOD_AUTO(OpenGLBackend, Clear, "Clear"),
        KL_METHOD_AUTO(OpenGLBackend, SetTitle, "Set title"),
        KL_METHOD_AUTO(OpenGLBackend, SetFullscreen, "Set fullscreen"),
        KL_METHOD_AUTO(OpenGLBackend, IsWindowOpen, "Is window open"),
        KL_METHOD_AUTO(OpenGLBackend, ProcessEvents, "Process events")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(OpenGLBackend)
        KL_CTOR(OpenGLBackend, uint32_t, uint32_t, const std::string&, bool, bool)
    KL_END_DESCRIBE(OpenGLBackend)
};

} // namespace koilo
