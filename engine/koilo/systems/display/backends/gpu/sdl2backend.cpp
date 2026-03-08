// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/display/backends/gpu/sdl2backend.hpp>
#include <cstring>
#include <iostream>
#include <SDL2/SDL.h>

namespace koilo {

SDL2Backend::SDL2Backend(uint32_t width, uint32_t height,
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
    info_.name = "SDL2: " + title;
    info_.refreshRate = 60;

    info_.AddCapability(DisplayCapability::RGB888);
    info_.AddCapability(DisplayCapability::HardwareScaling);
    info_.AddCapability(DisplayCapability::DoubleBuffering);
    info_.AddCapability(DisplayCapability::VSync);
}

SDL2Backend::~SDL2Backend() {
    Shutdown();
}

bool SDL2Backend::Initialize() {
    if (initialized_) {
        return true;
    }

    if (!InitSDL()) {
        std::cerr << "Failed to initialize SDL2" << std::endl;
        return false;
    }

    initialized_ = true;
    return true;
}

void SDL2Backend::Shutdown() {
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

bool SDL2Backend::IsInitialized() const {
    return initialized_;
}

DisplayInfo SDL2Backend::GetInfo() const {
    return info_;
}

bool SDL2Backend::HasCapability(DisplayCapability cap) const {
    return info_.HasCapability(cap);
}

bool SDL2Backend::Present(const Framebuffer& fb) {
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
            sdlFormat,
            SDL_TEXTUREACCESS_STREAMING,
            fb.width,
            fb.height
        );

        if (!texture_) {
            std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Debug: verify texture dimensions
        int actualWidth, actualHeight;
        uint32_t actualFormat;
        SDL_QueryTexture(texture_, &actualFormat, nullptr, &actualWidth, &actualHeight);
        
        const char* formatName = "UNKNOWN";
        if (actualFormat == SDL_PIXELFORMAT_RGB888) formatName = "RGB888";
        else if (actualFormat == SDL_PIXELFORMAT_RGBA8888) formatName = "RGBA8888";
        else if (actualFormat == SDL_PIXELFORMAT_BGR888) formatName = "BGR888";
        else if (actualFormat == SDL_PIXELFORMAT_BGRA8888) formatName = "BGRA8888";
        
        std::cout << "[SDL2Backend] Created texture: requested " << fb.width << "x" << fb.height 
                  << ", actual " << actualWidth << "x" << actualHeight 
                  << ", format " << formatName << " (0x" << std::hex << actualFormat << std::dec << ")\n";
    }

    // Upload pixel data to texture
    void* pixels;
    int pitch;
    if (SDL_LockTexture(texture_, nullptr, &pixels, &pitch) != 0) {
        std::cerr << "Failed to lock texture: " << SDL_GetError() << std::endl;
        return false;
    }

    // Debug: print first few pixels being uploaded to SDL
    static bool debugPrinted = false;
    if (!debugPrinted) {
        const uint8_t* data = static_cast<const uint8_t*>(fb.data);
        std::cout << "[SDL2Backend] Uploading framebuffer to SDL:\n";
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

    SDL_Rect dstRect = { vpX, vpY, vpWidth, vpHeight };

    // Render texture
    SDL_RenderCopy(renderer_, texture_, nullptr, &dstRect);

    // Present
    SDL_RenderPresent(renderer_);

    return true;
}

void SDL2Backend::WaitVSync() {
    // VSync is handled by SDL_RENDERER_PRESENTVSYNC flag
    // No explicit wait needed
}

bool SDL2Backend::Clear() {
    if (!initialized_) {
        return false;
    }

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);

    return true;
}

bool SDL2Backend::SetRefreshRate(uint32_t hz) {
    info_.refreshRate = hz;
    // SDL doesn't provide direct refresh rate control
    return true;
}

bool SDL2Backend::SetOrientation(Orientation orient) {
    // Not applicable for desktop windows
    (void)orient;
    return false;
}

bool SDL2Backend::SetBrightness(uint8_t brightness) {
    float b = brightness / 255.0f;
    if (window_) {
        return SDL_SetWindowBrightness(window_, b) == 0;
    }
    return false;
}

bool SDL2Backend::SetVSyncEnabled(bool enabled) {
    vsyncEnabled_ = enabled;
    // VSync is set during renderer creation
    // Would need to recreate renderer to change this
    return false;
}

void SDL2Backend::SetTitle(const std::string& title) {
    title_ = title;
    if (window_) {
        SDL_SetWindowTitle(window_, title.c_str());
    }
}

bool SDL2Backend::SetFullscreen(bool fullscreen) {
    if (!window_) {
        return false;
    }

    fullscreen_ = fullscreen;
    Uint32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    return SDL_SetWindowFullscreen(window_, flags) == 0;
}

bool SDL2Backend::IsWindowOpen() const {
    return initialized_ && !shouldClose_;
}

bool SDL2Backend::ProcessEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                shouldClose_ = true;
                return false;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    windowWidth_ = event.window.data1;
                    windowHeight_ = event.window.data2;
                }
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    shouldClose_ = true;
                    return false;
                }
                break;
        }
    }

    return true;
}

void SDL2Backend::GetWindowSize(uint32_t& width, uint32_t& height) const {
    width = windowWidth_;
    height = windowHeight_;
}

// === Private Helper Methods ===

bool SDL2Backend::InitSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create window
    Uint32 flags = SDL_WINDOW_SHOWN;
    if (resizable_) flags |= SDL_WINDOW_RESIZABLE;
    if (fullscreen_) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    window_ = SDL_CreateWindow(
        title_.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
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
    std::cout << "[SDL2Backend] Requested window: " << windowWidth_ << "x" << windowHeight_ << "\n";
    std::cout << "[SDL2Backend] Actual window: " << actualW << "x" << actualH << "\n";

    // Create renderer
    Uint32 rendererFlags = SDL_RENDERER_ACCELERATED;
    if (vsyncEnabled_) {
        rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, rendererFlags);
    if (!renderer_) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set logical size to match window size
    SDL_RenderSetLogicalSize(renderer_, windowWidth_, windowHeight_);

    return true;
}

void SDL2Backend::CalculateViewport(uint32_t fbWidth, uint32_t fbHeight,
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
                // Fit width
                width = windowWidth_;
                height = static_cast<int>(windowWidth_ / fbAspect);
                x = 0;
                y = (windowHeight_ - height) / 2;
            } else {
                // Fit height
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
                // Fill width
                width = windowWidth_;
                height = static_cast<int>(windowWidth_ / fbAspect);
                x = 0;
                y = (windowHeight_ - height) / 2;
            } else {
                // Fill height
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

uint32_t SDL2Backend::ConvertPixelFormat(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGB888:
            return SDL_PIXELFORMAT_RGB888;
        case PixelFormat::BGR888:
            return SDL_PIXELFORMAT_BGR888;
        case PixelFormat::RGBA8888:
            return SDL_PIXELFORMAT_RGBA8888;
        case PixelFormat::BGRA8888:
            return SDL_PIXELFORMAT_BGRA8888;
        case PixelFormat::RGB565:
            return SDL_PIXELFORMAT_RGB565;
        default:
            return SDL_PIXELFORMAT_RGB888;
    }
}

} // namespace koilo
