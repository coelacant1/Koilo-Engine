// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/display/backends/gpu/openglbackend.hpp>
#include <iostream>
#include <cstdlib>

// GPU preference hints - tell NVIDIA/AMD drivers to use discrete GPU
// These are checked by the driver at DLL/SO load time
#ifdef _WIN32
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#else
extern "C" {
    unsigned long NvOptimusEnablement __attribute__((visibility("default"))) = 1;
    int AmdPowerXpressRequestHighPerformance __attribute__((visibility("default"))) = 1;
}

// On Linux PRIME setups, set NVIDIA offload env vars as early as possible.
// Must happen before GLX libraries are loaded. Uses GCC constructor priority
// to run before most other static initializers.
__attribute__((constructor(101)))
static void koilo_request_discrete_gpu() {
    setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 0);
    setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 0);
}
#endif

// SDL3 and OpenGL includes
#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else
    #include <glad/glad.h>
#endif

#include <SDL3/SDL.h>

namespace koilo {

// Vertex shader source
static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

// Fragment shader source
static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D framebufferTexture;

void main()
{
    FragColor = texture(framebufferTexture, TexCoord);
}
)";

// Quad vertices (position + texcoord)
static const float quadVertices[] = {
    // Positions   // TexCoords
    -1.0f,  1.0f,  0.0f, 0.0f,  // Top-left
    -1.0f, -1.0f,  0.0f, 1.0f,  // Bottom-left
     1.0f, -1.0f,  1.0f, 1.0f,  // Bottom-right
    
    -1.0f,  1.0f,  0.0f, 0.0f,  // Top-left
     1.0f, -1.0f,  1.0f, 1.0f,  // Bottom-right
     1.0f,  1.0f,  1.0f, 0.0f   // Top-right
};

koilo::OpenGLBackend::OpenGLBackend(uint32_t width, uint32_t height,
                             const std::string& title,
                             bool fullscreen, bool resizable)
    : window_(nullptr), glContext_(nullptr),
      windowWidth_(width), windowHeight_(height),
      framebufferWidth_(width), framebufferHeight_(height),
      title_(title), fullscreen_(fullscreen), resizable_(resizable),
      initialized_(false), vsyncEnabled_(true), shouldClose_(false),
      scalingMode_(ScalingMode::Fit),
      texture_(0), vao_(0), vbo_(0), shaderProgram_(0) {
    
    info_.width = width;
    info_.height = height;
    info_.name = "OpenGL: " + title;
    info_.refreshRate = 60;

    info_.AddCapability(DisplayCapability::RGB888);
    info_.AddCapability(DisplayCapability::HardwareScaling);
    info_.AddCapability(DisplayCapability::GPUAcceleration);
    info_.AddCapability(DisplayCapability::DoubleBuffering);
    info_.AddCapability(DisplayCapability::VSync);
}

koilo::OpenGLBackend::~OpenGLBackend() {
    Shutdown();
}

bool koilo::OpenGLBackend::Initialize() {
    if (initialized_) {
        return true;
    }
    
    if (!InitSDL()) {
        std::cerr << "Failed to initialize SDL3" << std::endl;
        return false;
    }
    
    if (!InitOpenGL()) {
        std::cerr << "Failed to initialize OpenGL" << std::endl;
        Shutdown();
        return false;
    }
    
    if (!CreateShader()) {
        std::cerr << "Failed to create shader" << std::endl;
        Shutdown();
        return false;
    }
    
    initialized_ = true;
    return true;
}

void koilo::OpenGLBackend::Shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Cleanup OpenGL resources
    if (texture_) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (shaderProgram_) {
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
    }
    
    // Cleanup SDL
    if (glContext_) {
        SDL_GL_DestroyContext(glContext_);
        glContext_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    
    SDL_Quit();
    
    initialized_ = false;
}

bool koilo::OpenGLBackend::IsInitialized() const {
    return initialized_;
}

void koilo::OpenGLBackend::SetNearestFiltering(bool nearest) {
    if (!texture_) return;
    GLenum filter = nearest ? GL_NEAREST : GL_LINEAR;
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

DisplayInfo koilo::OpenGLBackend::GetInfo() const {
    return info_;
}

bool koilo::OpenGLBackend::HasCapability(DisplayCapability cap) const {
    return info_.HasCapability(cap);
}

bool koilo::OpenGLBackend::Present(const Framebuffer& fb) {
    if (!initialized_ || !fb.IsValid()) {
        return false;
    }
    
    // Store framebuffer dimensions
    framebufferWidth_ = fb.width;
    framebufferHeight_ = fb.height;
    
    // Upload texture
    glBindTexture(GL_TEXTURE_2D, texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // RGB888 rows are 3*W bytes, not 4-aligned
    
    GLenum format = GL_RGB;
    GLenum type = GL_UNSIGNED_BYTE;
    
    if (fb.format == PixelFormat::RGB888) {
        format = GL_RGB;
    } else if (fb.format == PixelFormat::RGBA8888) {
        format = GL_RGBA;
    } else if (fb.format == PixelFormat::BGR888) {
        format = GL_BGR;
    } else if (fb.format == PixelFormat::BGRA8888) {
        format = GL_BGRA;
    } else {
        // Unsupported format
        return false;
    }
    
    if (fb.width == texWidth_ && fb.height == texHeight_) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fb.width, fb.height,
                        format, type, fb.data);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fb.width, fb.height, 0,
                     format, type, fb.data);
        texWidth_ = fb.width;
        texHeight_ = fb.height;
    }
    
    // Calculate viewport based on scaling mode
    int vpX, vpY, vpWidth, vpHeight;
    CalculateViewport(fb.width, fb.height, vpX, vpY, vpWidth, vpHeight);
    
    // Clear and render
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glViewport(vpX, vpY, vpWidth, vpHeight);
    
    RenderQuad();
    
    // Swap buffers
    SDL_GL_SwapWindow(window_);
    
    return true;
}

void koilo::OpenGLBackend::SwapOnly() {
    if (window_) SDL_GL_SwapWindow(window_);
}

bool koilo::OpenGLBackend::PresentNoSwap(const Framebuffer& fb) {
    if (!initialized_ || !fb.IsValid()) return false;
    framebufferWidth_ = fb.width;
    framebufferHeight_ = fb.height;

    glBindTexture(GL_TEXTURE_2D, texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLenum format = GL_RGB;
    if (fb.format == PixelFormat::RGBA8888) format = GL_RGBA;
    else if (fb.format == PixelFormat::BGR888) format = GL_BGR;
    else if (fb.format == PixelFormat::BGRA8888) format = GL_BGRA;

    if (fb.width == texWidth_ && fb.height == texHeight_) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fb.width, fb.height,
                        format, GL_UNSIGNED_BYTE, fb.data);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fb.width, fb.height, 0,
                     format, GL_UNSIGNED_BYTE, fb.data);
        texWidth_ = fb.width;
        texHeight_ = fb.height;
    }

    int vpX, vpY, vpWidth, vpHeight;
    CalculateViewport(fb.width, fb.height, vpX, vpY, vpWidth, vpHeight);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(vpX, vpY, vpWidth, vpHeight);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    RenderQuad();
    return true;
}

void koilo::OpenGLBackend::WaitVSync() {
    // VSync is handled by SDL_GL_SetSwapInterval
}

bool koilo::OpenGLBackend::Clear() {
    if (!initialized_) {
        return false;
    }
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(window_);
    
    return true;
}

bool koilo::OpenGLBackend::SetRefreshRate(uint32_t hz) {
    info_.refreshRate = hz;
    // SDL doesn't provide direct refresh rate control
    return true;
}

bool koilo::OpenGLBackend::SetOrientation(Orientation orient) {
    // Not applicable for desktop windows
    (void)orient;
    return false;
}

bool koilo::OpenGLBackend::SetBrightness(uint8_t brightness) {
    // SDL3 removed SDL_SetWindowBrightness - brightness is managed by the OS
    (void)brightness;
    return false;
}

bool koilo::OpenGLBackend::SetVSyncEnabled(bool enabled) {
    vsyncEnabled_ = enabled;
    if (glContext_) {
        return SDL_GL_SetSwapInterval(enabled ? 1 : 0);
    }
    return false;
}

void koilo::OpenGLBackend::SetTitle(const std::string& title) {
    title_ = title;
    if (window_) {
        SDL_SetWindowTitle(window_, title.c_str());
    }
}

bool koilo::OpenGLBackend::SetFullscreen(bool fullscreen) {
    if (!window_) {
        return false;
    }
    
    fullscreen_ = fullscreen;
    return SDL_SetWindowFullscreen(window_, fullscreen);
}

bool koilo::OpenGLBackend::IsWindowOpen() const {
    return initialized_ && !shouldClose_;
}

bool koilo::OpenGLBackend::ProcessEvents() {
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

void koilo::OpenGLBackend::GetWindowSize(uint32_t& width, uint32_t& height) const {
    width = windowWidth_;
    height = windowHeight_;
}

// === Private Helper Methods ===

bool koilo::OpenGLBackend::InitSDL() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    // Create window
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL;
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

    // Enable text input events (required on some platforms for SDL_EVENT_TEXT_INPUT)
    SDL_StartTextInput(window_);
    
    // Create OpenGL context
    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_) {
        std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Set VSync
    SDL_GL_SetSwapInterval(vsyncEnabled_ ? 1 : 0);
    
    return true;
}

bool koilo::OpenGLBackend::InitOpenGL() {
    // Load GL function pointers via SDL (works on both GLX and EGL/Wayland)
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to load OpenGL functions via glad" << std::endl;
        return false;
    }
    
    // Create texture
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Create VAO and VBO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    return true;
}

bool koilo::OpenGLBackend::CreateShader() {
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
        return false;
    }
    
    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return false;
    }
    
    // Link shader program
    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vertexShader);
    glAttachShader(shaderProgram_, fragmentShader);
    glLinkProgram(shaderProgram_);
    
    glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram_, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return true;
}

void koilo::OpenGLBackend::RenderQuad() {
    glUseProgram(shaderProgram_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void koilo::OpenGLBackend::CalculateViewport(uint32_t fbWidth, uint32_t fbHeight,
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
            x = (windowWidth_ - fbWidth) / 2;
            y = (windowHeight_ - fbHeight) / 2;
            width = fbWidth;
            height = fbHeight;
            break;
    }
}

} // namespace koilo
