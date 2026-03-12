// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/display/backends/gpu/sdl3backend.hpp>
#include <cstring>
#include <iostream>
#include <SDL3/SDL.h>

namespace koilo {

SDL3Backend::SDL3Backend(uint32_t width, uint32_t height,
                         const std::string& title,
                         bool fullscreen, bool resizable)
    : window_(nullptr), renderer_(nullptr), texture_(nullptr),
      windowWidth_(width), windowHeight_(height),
      framebufferWidth_(0), framebufferHeight_(0),
      title_(title), fullscreen_(fullscreen), resizable_(resizable),
      initialized_(false), vsyncEnabled_(true), shouldClose_(false),
      scalingMode_(ScalingMode::Fit) {

    info_.width = width;
    info_.height = height;
    info_.name = "SDL3: " + title;
    info_.refreshRate = 60;

    info_.AddCapability(DisplayCapability::RGB888);
    info_.AddCapability(DisplayCapability::HardwareScaling);
    info_.AddCapability(DisplayCapability::DoubleBuffering);
    info_.AddCapability(DisplayCapability::VSync);
}

SDL3Backend::~SDL3Backend() {
    Shutdown();
}

bool SDL3Backend::Initialize() {
    if (initialized_) {
        return true;
    }

    if (!InitSDL()) {
        std::cerr << "Failed to initialize SDL3" << std::endl;
        return false;
    }

    initialized_ = true;
    return true;
}

void SDL3Backend::Shutdown() {
    if (!initialized_) {
        return;
    }

    // Cleanup SDL resources
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();

    initialized_ = false;
}

bool SDL3Backend::IsInitialized() const {
    return initialized_;
}

DisplayInfo SDL3Backend::GetInfo() const {
    return info_;
}

bool SDL3Backend::HasCapability(DisplayCapability cap) const {
    return info_.HasCapability(cap);
}

bool SDL3Backend::Present(const Framebuffer& fb) {
    if (!initialized_ || !fb.IsValid()) {
        return false;
    }

    // Create or recreate texture if dimensions changed
    if (!texture_ || fb.width != framebufferWidth_ || fb.height != framebufferHeight_) {
        framebufferWidth_ = fb.width;
        framebufferHeight_ = fb.height;

        // Destroy old texture if it exists
        if (texture_) {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }

        // Create new texture
        uint32_t sdlFormat = ConvertPixelFormat(fb.format);
        texture_ = SDL_CreateTexture(
            renderer_,
            static_cast<SDL_PixelFormat>(sdlFormat),
            SDL_TEXTUREACCESS_STREAMING,
            fb.width,
            fb.height
        );

        if (!texture_) {
            std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Debug: verify texture dimensions
        float actualW, actualH;
        SDL_GetTextureSize(texture_, &actualW, &actualH);
        
        std::cout << "[SDL3Backend] Created texture: requested " << fb.width << "x" << fb.height 
                  << ", actual " << static_cast<int>(actualW) << "x" << static_cast<int>(actualH) << "\n";
    }

    // Upload pixel data to texture
    void* pixels;
    int pitch;
    if (!SDL_LockTexture(texture_, nullptr, &pixels, &pitch)) {
        std::cerr << "Failed to lock texture: " << SDL_GetError() << std::endl;
        return false;
    }

    // Debug: print first few pixels being uploaded to SDL
    static bool debugPrinted = false;
    if (!debugPrinted) {
        const uint8_t* data = static_cast<const uint8_t*>(fb.data);
        std::cout << "[SDL3Backend] Uploading framebuffer to SDL:\n";
        std::cout << "  Dimensions: " << fb.width << "x" << fb.height << "\n";
        std::cout << "  Format: " << (fb.format == PixelFormat::RGB888 ? "RGB888" : "BGR888") << "\n";
        std::cout << "  Stride: " << fb.stride << ", SDL pitch: " << pitch << "\n";
        std::cout << "  First pixel (0,0) bytes: [" << (int)data[0] << "," << (int)data[1] << "," << (int)data[2] << "]\n";
        std::cout << "  Pixel at index 799 bytes: [" << (int)data[799*3] << "," << (int)data[799*3+1] << "," << (int)data[799*3+2] << "]\n";
        debugPrinted = true;
    }

    // Copy framebuffer data to texture
    if (pitch == static_cast<int>(fb.stride)) {
        // Fast path - direct copy
        std::memcpy(pixels, fb.data, fb.GetSizeBytes());
    } else {
        // Slow path - copy row by row
        uint8_t* dst = static_cast<uint8_t*>(pixels);
        const uint8_t* src = static_cast<const uint8_t*>(fb.data);
        uint32_t bytesPerRow = fb.width * GetBytesPerPixel(fb.format);

        for (uint32_t y = 0; y < fb.height; ++y) {
            std::memcpy(dst, src, bytesPerRow);
            dst += pitch;
            src += fb.stride;
        }
    }

    SDL_UnlockTexture(texture_);

    // Clear renderer
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    // Calculate viewport based on scaling mode
    int vpX, vpY, vpWidth, vpHeight;
    CalculateViewport(fb.width, fb.height, vpX, vpY, vpWidth, vpHeight);

    SDL_FRect dstRect = { static_cast<float>(vpX), static_cast<float>(vpY),
                          static_cast<float>(vpWidth), static_cast<float>(vpHeight) };

    // Render texture
    SDL_RenderTexture(renderer_, texture_, nullptr, &dstRect);

    // Present
    SDL_RenderPresent(renderer_);

    return true;
}

void SDL3Backend::WaitVSync() {
    // VSync is handled by SDL_SetRenderVSync
}

bool SDL3Backend::Clear() {
    if (!initialized_) {
        return false;
    }

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);

    return true;
}

bool SDL3Backend::SetRefreshRate(uint32_t hz) {
    info_.refreshRate = hz;
    return true;
}

bool SDL3Backend::SetOrientation(Orientation orient) {
    (void)orient;
    return false;
}

bool SDL3Backend::SetBrightness(uint8_t brightness) {
    // SDL3 removed SDL_SetWindowBrightness - brightness is managed by the OS
    (void)brightness;
    return false;
}

bool SDL3Backend::SetVSyncEnabled(bool enabled) {
    vsyncEnabled_ = enabled;
    if (renderer_) {
        return SDL_SetRenderVSync(renderer_, enabled ? 1 : 0);
    }
    return false;
}

void SDL3Backend::SetTitle(const std::string& title) {
    title_ = title;
    if (window_) {
        SDL_SetWindowTitle(window_, title.c_str());
    }
}

bool SDL3Backend::SetFullscreen(bool fullscreen) {
    if (!window_) {
        return false;
    }

    fullscreen_ = fullscreen;
    return SDL_SetWindowFullscreen(window_, fullscreen);
}

bool SDL3Backend::IsWindowOpen() const {
    return initialized_ && !shouldClose_;
}

bool SDL3Backend::ProcessEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                shouldClose_ = true;
                return false;

            case SDL_EVENT_WINDOW_RESIZED:
                windowWidth_ = event.window.data1;
                windowHeight_ = event.window.data2;
                break;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) {
                    shouldClose_ = true;
                    return false;
                }
                break;
        }
    }

    return true;
}

void SDL3Backend::GetWindowSize(uint32_t& width, uint32_t& height) const {
    width = windowWidth_;
    height = windowHeight_;
}

// === Private Helper Methods ===

bool SDL3Backend::InitSDL() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create window
    SDL_WindowFlags flags = 0;
    if (resizable_) flags |= SDL_WINDOW_RESIZABLE;
    if (fullscreen_) flags |= SDL_WINDOW_FULLSCREEN;

    window_ = SDL_CreateWindow(
        title_.c_str(),
        windowWidth_,
        windowHeight_,
        flags
    );

    if (!window_) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Debug: Verify actual window size
    int actualW, actualH;
    SDL_GetWindowSize(window_, &actualW, &actualH);
    std::cout << "[SDL3Backend] Requested window: " << windowWidth_ << "x" << windowHeight_ << "\n";
    std::cout << "[SDL3Backend] Actual window: " << actualW << "x" << actualH << "\n";

    // Create renderer
    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!renderer_) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set VSync
    SDL_SetRenderVSync(renderer_, vsyncEnabled_ ? 1 : 0);

    // Set logical size to match window size
    SDL_SetRenderLogicalPresentation(renderer_, windowWidth_, windowHeight_,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);

    return true;
}

void SDL3Backend::CalculateViewport(uint32_t fbWidth, uint32_t fbHeight,
                                    int& x, int& y, int& width, int& height) {
    switch (scalingMode_) {
        case ScalingMode::Stretch:
            x = 0;
            y = 0;
            width = windowWidth_;
            height = windowHeight_;
            break;

        case ScalingMode::Fit: {
            float fbAspect = static_cast<float>(fbWidth) / fbHeight;
            float winAspect = static_cast<float>(windowWidth_) / windowHeight_;

            if (fbAspect > winAspect) {
                width = windowWidth_;
                height = static_cast<int>(windowWidth_ / fbAspect);
                x = 0;
                y = (windowHeight_ - height) / 2;
            } else {
                height = windowHeight_;
                width = static_cast<int>(windowHeight_ * fbAspect);
                x = (windowWidth_ - width) / 2;
                y = 0;
            }
            break;
        }

        case ScalingMode::Fill: {
            float fbAspect = static_cast<float>(fbWidth) / fbHeight;
            float winAspect = static_cast<float>(windowWidth_) / windowHeight_;

            if (fbAspect < winAspect) {
                width = windowWidth_;
                height = static_cast<int>(windowWidth_ / fbAspect);
                x = 0;
                y = (windowHeight_ - height) / 2;
            } else {
                height = windowHeight_;
                width = static_cast<int>(windowHeight_ * fbAspect);
                x = (windowWidth_ - width) / 2;
                y = 0;
            }
            break;
        }

        case ScalingMode::None:
            x = (static_cast<int>(windowWidth_) - static_cast<int>(fbWidth)) / 2;
            y = (static_cast<int>(windowHeight_) - static_cast<int>(fbHeight)) / 2;
            width = fbWidth;
            height = fbHeight;
            break;
    }
}

uint32_t SDL3Backend::ConvertPixelFormat(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGB888:
            return SDL_PIXELFORMAT_XRGB8888;
        case PixelFormat::BGR888:
            return SDL_PIXELFORMAT_XBGR8888;
        case PixelFormat::RGBA8888:
            return SDL_PIXELFORMAT_RGBA8888;
        case PixelFormat::BGRA8888:
            return SDL_PIXELFORMAT_BGRA8888;
        case PixelFormat::RGB565:
            return SDL_PIXELFORMAT_RGB565;
        default:
            return SDL_PIXELFORMAT_XRGB8888;
    }
}

} // namespace koilo
