// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file opengl_render_backend.cpp
 * @brief OpenGL 3.3+ GPU render backend implementation.
 *
 * Renders scenes to an FBO using vertex/fragment shaders, then
 * provides ReadPixels for CPU compositing (Canvas2D, modules, debug).
 *
 * @date 03/03/2026
 * @author Coela
 */

#include <koilo/systems/render/gl/opengl_render_backend.hpp>
#include <koilo/systems/scene/scene.hpp>
#include <koilo/systems/scene/mesh.hpp>
#include <koilo/systems/scene/camera/camerabase.hpp>
#include <koilo/systems/scene/camera/camera.hpp>
#include <koilo/systems/scene/camera/cameralayout.hpp>
#include <koilo/systems/render/sky/sky.hpp>
#include <koilo/systems/render/material/imaterial.hpp>
#include <koilo/systems/render/shader/ishader.hpp>
#include <koilo/systems/render/material/implementations/kslmaterial.hpp>
#include <koilo/ksl/ksl_symbols.hpp>
#include <koilo/systems/scene/lighting/light.hpp>
#include <koilo/assets/image/texture.hpp>
#include <koilo/core/math/matrix4x4.hpp>
#include <koilo/core/math/transform.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/vector2d.hpp>
#include <koilo/assets/model/itrianglegroup.hpp>
#include <koilo/core/geometry/3d/triangle.hpp>
#include <koilo/assets/model/indexgroup.hpp>
#include <koilo/systems/profiling/performanceprofiler.hpp>
#include <koilo/debug/debugdraw.hpp>
#include <koilo/systems/render/canvas2d.hpp>

#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>

// OpenGL includes
#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else
    #include <glad/glad.h>
#endif

namespace koilo {

// ============================================================================
// GLSL Shader Sources
// ============================================================================

// --- Shared vertex shader for all scene geometry ---
static const char* s_sceneVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec3 u_cameraPos;

out vec3 v_position;
out vec3 v_normal;
out vec2 v_uv;
out vec3 v_viewDir;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    v_position = worldPos.xyz;
    v_normal = normalize(mat3(u_model) * a_normal);
    v_uv = a_uv;
    v_viewDir = normalize(worldPos.xyz - u_cameraPos);
    gl_Position = u_projection * u_view * worldPos;
}
)";

// --- Pink error fallback fragment shader ---
static const char* s_pinkErrorFragSrc = R"(
#version 330 core
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0, 0.0, 0.78, 1.0);
}
)";

// ============================================================================
// Sky quad vertices (pos3, normal3, uv2 - matches standard vertex layout)
// ============================================================================

static const float s_skyQuadVerts[] = {
    // pos               normal          uv
    -1.0f, -1.0f, 0.999f,  0,0,1,  0.0f, 0.0f,
     1.0f, -1.0f, 0.999f,  0,0,1,  1.0f, 0.0f,
     1.0f,  1.0f, 0.999f,  0,0,1,  1.0f, 1.0f,
    -1.0f, -1.0f, 0.999f,  0,0,1,  0.0f, 0.0f,
     1.0f,  1.0f, 0.999f,  0,0,1,  1.0f, 1.0f,
    -1.0f,  1.0f, 0.999f,  0,0,1,  0.0f, 1.0f,
};

// ============================================================================
// Interleaved vertex for mesh upload
// ============================================================================

struct GLVertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
};

// ============================================================================
// Shader registry helpers
// ============================================================================

static inline void SetUniform3c(GLuint prog, const char* name, const Color888& c) {
    glUniform3f(glGetUniformLocation(prog, name),
                c.R / 255.0f, c.G / 255.0f, c.B / 255.0f);
}

static inline void SetUniform3v(GLuint prog, const char* name, const Vector3D& v) {
    glUniform3f(glGetUniformLocation(prog, name), v.X, v.Y, v.Z);
}

// --- Debug line shaders ---
static const char* s_lineVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
uniform mat4 u_view;
uniform mat4 u_projection;
out vec4 v_color;
void main() {
    v_color = a_color;
    gl_Position = u_projection * u_view * vec4(a_position, 1.0);
}
)";

static const char* s_lineFragSrc = R"(
#version 330 core
in vec4 v_color;
out vec4 FragColor;
void main() {
    FragColor = v_color;
}
)";

// --- Canvas overlay shaders ---
static const char* s_overlayVertSrc = R"(
#version 330 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
out vec2 v_uv;
void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_position, 0.0, 1.0);
}
)";

static const char* s_overlayFragSrc = R"(
#version 330 core
in vec2 v_uv;
uniform sampler2D u_texture;
out vec4 FragColor;
void main() {
    FragColor = texture(u_texture, v_uv);
}
)";

// ============================================================================
// Implementation
// ============================================================================

OpenGLRenderBackend::OpenGLRenderBackend()
    : fbo_(0), colorTex_(0), depthRbo_(0),
      fbWidth_(0), fbHeight_(0),
      sceneProgram_(0), pinkProgram_(0),
      skyVao_(0), skyVbo_(0),
      overlayProgram_(0), overlayVao_(0), overlayVbo_(0),
      overlayTex_(0), overlayTexW_(0), overlayTexH_(0),
      blitVao_(0), blitVbo_(0),
      lineProgram_(0), lineVao_(0), lineVbo_(0),
      batchVao_(0), batchVbo_(0), batchVboSize_(0),
      pbo_{0, 0}, pboIndex_(0), pboReady_(false),
      initialized_(false) {}

const OpenGLRenderBackend::UniformLocs& OpenGLRenderBackend::GetUniforms(unsigned int prog) {
    auto it = uniformCache_.find(prog);
    if (it != uniformCache_.end()) return it->second;
    UniformLocs& u = uniformCache_[prog];
    u.model      = glGetUniformLocation(prog, "u_model");
    u.view       = glGetUniformLocation(prog, "u_view");
    u.projection = glGetUniformLocation(prog, "u_projection");
    u.time       = glGetUniformLocation(prog, "u_time");
    u.cameraPos  = glGetUniformLocation(prog, "u_cameraPos");
    u.lightCount = glGetUniformLocation(prog, "u_lightCount");
    return u;
}

int OpenGLRenderBackend::GetKSLUniform(unsigned int prog, const std::string& name) {
    auto& progCache = kslUniformCache_[prog];
    auto it = progCache.find(name);
    if (it != progCache.end()) return it->second;
    int loc = glGetUniformLocation(prog, name.c_str());
    progCache[name] = loc;
    return loc;
}

OpenGLRenderBackend::~OpenGLRenderBackend() {
    Shutdown();
}

bool OpenGLRenderBackend::Initialize() {
    if (initialized_) return true;

    // Verify GL context is available
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (!version) {
        std::cerr << "[OpenGLRenderBackend] No OpenGL context available. "
                  << "Ensure OpenGL display backend is initialized first." << std::endl;
        return false;
    }
    std::cout << "[OpenGLRenderBackend] OpenGL version: " << version << std::endl;

    // Detect software renderers and suggest discrete GPU activation
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    if (renderer) {
        std::string r(renderer);
        if (r.find("llvmpipe") != std::string::npos ||
            r.find("softpipe") != std::string::npos ||
            r.find("swrast") != std::string::npos) {
            std::cerr << "[OpenGLRenderBackend] Warning: Running on software renderer (" << renderer << ").\n"
                      << "  For NVIDIA: __NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./program\n"
                      << "  For Mesa:   DRI_PRIME=1 ./program" << std::endl;
        }
    }

    if (!InitShaders()) {
        std::cerr << "[OpenGLRenderBackend] Failed to compile shaders." << std::endl;
        return false;
    }

    InitShaderRegistry();

    // Create sky quad VAO/VBO (pos3, normal3, uv2 = 8 floats/vertex)
    glGenVertexArrays(1, &skyVao_);
    glGenBuffers(1, &skyVbo_);
    glBindVertexArray(skyVao_);
    glBindBuffer(GL_ARRAY_BUFFER, skyVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_skyQuadVerts), s_skyQuadVerts, GL_STATIC_DRAW);
    const int stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // Create debug line program + VAO/VBO
    lineProgram_ = CompileProgram(s_lineVertSrc, s_lineFragSrc);
    glGenVertexArrays(1, &lineVao_);
    glGenBuffers(1, &lineVbo_);
    glBindVertexArray(lineVao_);
    glBindBuffer(GL_ARRAY_BUFFER, lineVbo_);
    // pos3 + color4 = 7 floats per vertex
    const int lineStride = 7 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, lineStride, nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, lineStride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Create canvas overlay program + quad VAO/VBO
    overlayProgram_ = CompileProgram(s_overlayVertSrc, s_overlayFragSrc);
    glGenVertexArrays(1, &overlayVao_);
    glGenBuffers(1, &overlayVbo_);
    glBindVertexArray(overlayVao_);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVbo_);
    // Fullscreen quad: pos2 + uv2 = 4 floats × 6 vertices
    float quadVerts[] = {
        -1.f, -1.f,  0.f, 1.f,
         1.f, -1.f,  1.f, 1.f,
         1.f,  1.f,  1.f, 0.f,
        -1.f, -1.f,  0.f, 1.f,
         1.f,  1.f,  1.f, 0.f,
        -1.f,  1.f,  0.f, 0.f,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    const int overlayStride = 4 * sizeof(float);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, overlayStride, nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, overlayStride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    // Texture created lazily on first use

    // Create FBO-to-screen blit quad (same shader, non-flipped UVs)
    glGenVertexArrays(1, &blitVao_);
    glGenBuffers(1, &blitVbo_);
    glBindVertexArray(blitVao_);
    glBindBuffer(GL_ARRAY_BUFFER, blitVbo_);
    // FBO textures are bottom-up in GL, so v=0 at bottom, v=1 at top
    float blitQuadVerts[] = {
        -1.f, -1.f,  0.f, 0.f,
         1.f, -1.f,  1.f, 0.f,
         1.f,  1.f,  1.f, 1.f,
        -1.f, -1.f,  0.f, 0.f,
         1.f,  1.f,  1.f, 1.f,
        -1.f,  1.f,  0.f, 1.f,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(blitQuadVerts), blitQuadVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, overlayStride, nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, overlayStride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Create batch rendering VAO/VBO for merged draw calls
    glGenVertexArrays(1, &batchVao_);
    glGenBuffers(1, &batchVbo_);
    glBindVertexArray(batchVao_);
    glBindBuffer(GL_ARRAY_BUFFER, batchVbo_);
    // Same layout as GLVertex: pos3 + normal3 + uv2
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertex),
                          reinterpret_cast<void*>(offsetof(GLVertex, px)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertex),
                          reinterpret_cast<void*>(offsetof(GLVertex, nx)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex),
                          reinterpret_cast<void*>(offsetof(GLVertex, u)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // Initialize fallback for unsupported shaders
    fallback_.Initialize();

    initialized_ = true;
    std::cout << "[OpenGLRenderBackend] Initialized successfully." << std::endl;
    return true;
}

void OpenGLRenderBackend::Shutdown() {
    if (!initialized_) return;

    CleanupMeshCache();
    CleanupTextureCache();
    kslRegistry_.Clear();

    if (fbo_) { glDeleteFramebuffers(1, &fbo_); fbo_ = 0; }
    if (colorTex_) { glDeleteTextures(1, &colorTex_); colorTex_ = 0; }
    if (depthRbo_) { glDeleteRenderbuffers(1, &depthRbo_); depthRbo_ = 0; }

    if (sceneProgram_) { glDeleteProgram(sceneProgram_); sceneProgram_ = 0; }
    if (pinkProgram_) { glDeleteProgram(pinkProgram_); pinkProgram_ = 0; }

    if (skyVao_) { glDeleteVertexArrays(1, &skyVao_); skyVao_ = 0; }
    if (skyVbo_) { glDeleteBuffers(1, &skyVbo_); skyVbo_ = 0; }

    if (lineProgram_) { glDeleteProgram(lineProgram_); lineProgram_ = 0; }
    if (lineVao_) { glDeleteVertexArrays(1, &lineVao_); lineVao_ = 0; }
    if (lineVbo_) { glDeleteBuffers(1, &lineVbo_); lineVbo_ = 0; }

    if (overlayProgram_) { glDeleteProgram(overlayProgram_); overlayProgram_ = 0; }
    if (overlayVao_) { glDeleteVertexArrays(1, &overlayVao_); overlayVao_ = 0; }
    if (overlayVbo_) { glDeleteBuffers(1, &overlayVbo_); overlayVbo_ = 0; }

    if (blitVao_) { glDeleteVertexArrays(1, &blitVao_); blitVao_ = 0; }
    if (blitVbo_) { glDeleteBuffers(1, &blitVbo_); blitVbo_ = 0; }

    if (batchVao_) { glDeleteVertexArrays(1, &batchVao_); batchVao_ = 0; }
    if (batchVbo_) { glDeleteBuffers(1, &batchVbo_); batchVbo_ = 0; }
    batchVboSize_ = 0;
    if (overlayTex_) { glDeleteTextures(1, &overlayTex_); overlayTex_ = 0; }

    for (int i = 0; i < 2; ++i) {
        if (pbo_[i]) { glDeleteBuffers(1, &pbo_[i]); pbo_[i] = 0; }
    }
    pboReady_ = false;

    fallback_.Shutdown();
    initialized_ = false;
}

bool OpenGLRenderBackend::IsInitialized() const {
    return initialized_;
}

const char* OpenGLRenderBackend::GetName() const {
    return "OpenGL 3.3";
}

// ============================================================================
// Shader compilation
// ============================================================================

unsigned int OpenGLRenderBackend::CompileProgram(const char* vertSrc, const char* fragSrc) {
    auto compileShader = [](GLenum type, const char* src) -> GLuint {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint ok;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::cerr << "[OpenGLRenderBackend] Shader compile error:\n" << log << std::endl;
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    };

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc);
    if (!vs) return 0;

    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!fs) { glDeleteShader(vs); return 0; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "[OpenGLRenderBackend] Program link error:\n" << log << std::endl;
        glDeleteProgram(prog);
        prog = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

bool OpenGLRenderBackend::InitShaders() {
    // Pink error fallback uses scene vertex shader
    pinkProgram_ = CompileProgram(s_sceneVertSrc, s_pinkErrorFragSrc);
    if (!pinkProgram_) return false;

    // Default scene program starts as pink error (overridden by KSL)
    sceneProgram_ = pinkProgram_;

    return true;
}

// ============================================================================
// Shader registry - maps IShader type -> GLSL program + material binder
// ============================================================================

void OpenGLRenderBackend::SetLightUniforms(unsigned int prog, const std::vector<Light>& lights) {
    int count = static_cast<int>(std::min(lights.size(), static_cast<size_t>(16)));
    glUniform1i(glGetUniformLocation(prog, "u_lightCount"), count);

    for (int i = 0; i < count; ++i) {
        Light& l = const_cast<Light&>(lights[i]);
        char buf[64];

        // KSL struct-style uniforms: u_lights[i].field
        snprintf(buf, sizeof(buf), "u_lights[%d].position", i);
        SetUniform3v(prog, buf, l.GetPosition());

        snprintf(buf, sizeof(buf), "u_lights[%d].color", i);
        SetUniform3v(prog, buf, l.GetIntensity());

        snprintf(buf, sizeof(buf), "u_lights[%d].intensity", i);
        glUniform1f(glGetUniformLocation(prog, buf), 1.0f);

        snprintf(buf, sizeof(buf), "u_lights[%d].falloff", i);
        glUniform1f(glGetUniformLocation(prog, buf), l.GetFalloff());

        snprintf(buf, sizeof(buf), "u_lights[%d].curve", i);
        glUniform1f(glGetUniformLocation(prog, buf), l.GetCurveA());
    }
}

// Set light uniforms from KSL LightData (used by KSLMaterial)
static void SetKSLLightUniforms(unsigned int prog, const std::vector<ksl::LightData>& lights,
                                OpenGLRenderBackend& backend) {
    int count = static_cast<int>(std::min(lights.size(), static_cast<size_t>(16)));
    glUniform1i(backend.GetKSLUniform(prog, "u_lightCount"), count);

    for (int i = 0; i < count; ++i) {
        const auto& ld = lights[i];
        char buf[64];
        snprintf(buf, sizeof(buf), "u_lights[%d].position", i);
        glUniform3f(backend.GetKSLUniform(prog, buf), ld.position.x, ld.position.y, ld.position.z);
        snprintf(buf, sizeof(buf), "u_lights[%d].color", i);
        glUniform3f(backend.GetKSLUniform(prog, buf), ld.color.x, ld.color.y, ld.color.z);
        snprintf(buf, sizeof(buf), "u_lights[%d].intensity", i);
        glUniform1f(backend.GetKSLUniform(prog, buf), ld.intensity);
        snprintf(buf, sizeof(buf), "u_lights[%d].falloff", i);
        glUniform1f(backend.GetKSLUniform(prog, buf), ld.falloff);
        snprintf(buf, sizeof(buf), "u_lights[%d].curve", i);
        glUniform1f(backend.GetKSLUniform(prog, buf), ld.curve);
    }
}

// Auto-bind KSLMaterial params as GL uniforms via introspection
static void BindKSLMaterialUniforms(unsigned int prog, const KSLMaterial& kmat,
                                    OpenGLRenderBackend& backend) {
    ksl::KSLModule* mod = kmat.GetModule();
    void* inst = kmat.GetInstance();
    if (!mod || !inst) return;

    ksl::ParamList params = mod->GetParams();
    for (int i = 0; i < params.count; ++i) {
        const ksl::ParamDecl& decl = params.decls[i];
        std::string uName = std::string("u_") + decl.name;
        GLint loc = backend.GetKSLUniform(prog, uName);
        if (loc < 0) continue;

        const void* valPtr = static_cast<const char*>(inst) + decl.offset;
        const bool isArray = (decl.flags == ksl::ParamFlags::Array && decl.arraySize > 1);

        if (isArray) {
            for (int ai = 0; ai < decl.arraySize; ++ai) {
                std::string elemName = uName + "[" + std::to_string(ai) + "]";
                GLint eloc = backend.GetKSLUniform(prog, elemName);
                if (eloc < 0) continue;
                switch (decl.type) {
                    case ksl::ParamType::Float:
                        glUniform1f(eloc, static_cast<const float*>(valPtr)[ai]);
                        break;
                    case ksl::ParamType::Int:
                    case ksl::ParamType::Bool:
                        glUniform1i(eloc, static_cast<const int*>(valPtr)[ai]);
                        break;
                    case ksl::ParamType::Vec3: {
                        const auto* v = static_cast<const ksl::vec3*>(valPtr) + ai;
                        glUniform3f(eloc, v->x, v->y, v->z);
                        break;
                    }
                    default: break;
                }
            }
        } else {
            switch (decl.type) {
                case ksl::ParamType::Float:
                    glUniform1f(loc, *static_cast<const float*>(valPtr));
                    break;
                case ksl::ParamType::Int:
                case ksl::ParamType::Bool:
                    glUniform1i(loc, *static_cast<const int*>(valPtr));
                    break;
                case ksl::ParamType::Vec2: {
                    const auto* v = static_cast<const ksl::vec2*>(valPtr);
                    glUniform2f(loc, v->x, v->y);
                    break;
                }
                case ksl::ParamType::Vec3: {
                    const auto* v = static_cast<const ksl::vec3*>(valPtr);
                    glUniform3f(loc, v->x, v->y, v->z);
                    break;
                }
                case ksl::ParamType::Vec4: {
                    const auto* v = static_cast<const ksl::vec4*>(valPtr);
                    glUniform4f(loc, v->x, v->y, v->z, v->w);
                    break;
                }
            }
        }
    }

    // Lights
    SetKSLLightUniforms(prog, kmat.GetLights(), backend);

    // Camera
    const auto& cam = kmat.GetCameraPos();
    GLint camLoc = backend.GetKSLUniform(prog, "u_cameraPos");
    if (camLoc >= 0)
        glUniform3f(camLoc, cam.x, cam.y, cam.z);
}

void OpenGLRenderBackend::BindKSLTextures(unsigned int prog, const KSLMaterial& kmat) {
    for (int i = 0; i < kmat.TextureCount(); ++i) {
        Texture* tex = kmat.GetTexture(i);
        if (!tex) continue;
        std::string texName = "u_texture" + std::to_string(i);
        GLint texLoc = GetKSLUniform(prog, texName);
        if (texLoc < 0) continue;
        unsigned int texId = UploadTexture(tex);
        if (texId) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, texId);
            glUniform1i(texLoc, i);
        }
    }
}

unsigned int OpenGLRenderBackend::UploadTexture(Texture* tex) {
    if (!tex) return 0;
    uintptr_t key = reinterpret_cast<uintptr_t>(tex);
    auto it = textureCache_.find(key);
    if (it != textureCache_.end()) return it->second;

    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    uint32_t w = tex->GetWidth();
    uint32_t h = tex->GetHeight();

    if (tex->GetFormat() == Texture::Format::RGB888) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, tex->GetPixels());
    } else {
        // Palette mode: expand to RGB
        const uint8_t* indices = tex->GetIndices();
        const uint8_t* palette = tex->GetPalette();
        std::vector<uint8_t> rgb(w * h * 3);
        for (uint32_t i = 0; i < w * h; ++i) {
            uint8_t idx = indices[i];
            rgb[i * 3]     = palette[idx * 3];
            rgb[i * 3 + 1] = palette[idx * 3 + 1];
            rgb[i * 3 + 2] = palette[idx * 3 + 2];
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
    }

    textureCache_[key] = texId;
    return texId;
}

void OpenGLRenderBackend::CleanupTextureCache() {
    for (auto& pair : textureCache_) {
        glDeleteTextures(1, &pair.second);
    }
    textureCache_.clear();
}

void OpenGLRenderBackend::InitShaderRegistry() {
    // Load KSL-generated GLSL shaders from build/shaders/ directory
    // Try paths relative to executable: ./shaders/, ../shaders/, then absolute
    std::vector<std::string> searchPaths = {
        "shaders",
        "../shaders",
        "build/shaders"
    };

    std::string vertSrc(s_sceneVertSrc);
    ksl::KSLSymbolTable symbols;
    symbols.RegisterAll();
    int kslCount = 0;
    for (const auto& path : searchPaths) {
        kslCount = kslRegistry_.ScanDirectory(path, vertSrc, &symbols);
        if (kslCount > 0) {
            std::cout << "[OpenGLRenderBackend] Loaded " << kslCount
                      << " KSL shaders from " << path << "/" << std::endl;
            break;
        }
    }

    if (kslCount == 0) {
        std::cerr << "[OpenGLRenderBackend] WARNING: No KSL shaders found. "
                  << "All materials will use pink error fallback." << std::endl;
    }

    if (kslRegistry_.HasUberShader()) {
        std::cout << "[OpenGLRenderBackend] Uber-shader active ("
                  << kslCount << " shaders in single program)." << std::endl;
    }

    // Make registry available to KSLMaterial for script-based shader binding
    KSLMaterial::SetRegistry(&kslRegistry_);

    // Set sceneProgram_ to uniform_color KSL module (or pink fallback)
    ksl::KSLModule* ucMod = kslRegistry_.GetModule("uniform_color");
    if (ucMod && ucMod->HasGPU()) {
        sceneProgram_ = ucMod->GetGLProgram();
    }

    std::cout << "[OpenGLRenderBackend] Registered " << kslCount
              << " shader programs (KSL)." << std::endl;
}

// ============================================================================
// FBO management
// ============================================================================

bool OpenGLRenderBackend::InitFBO(int width, int height) {
    // Color texture
    glGenTextures(1, &colorTex_);
    glBindTexture(GL_TEXTURE_2D, colorTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Depth renderbuffer
    glGenRenderbuffers(1, &depthRbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

    // FBO
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex_, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRbo_);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[OpenGLRenderBackend] FBO incomplete: 0x" << std::hex << status << std::endl;
        return false;
    }

    fbWidth_ = width;
    fbHeight_ = height;
    return true;
}

void OpenGLRenderBackend::ResizeFBO(int width, int height) {
    if (fbo_) {
        glDeleteFramebuffers(1, &fbo_); fbo_ = 0;
        glDeleteTextures(1, &colorTex_); colorTex_ = 0;
        glDeleteRenderbuffers(1, &depthRbo_); depthRbo_ = 0;
    }
    // Recreate PBOs for new size
    for (int i = 0; i < 2; ++i) {
        if (pbo_[i]) { glDeleteBuffers(1, &pbo_[i]); pbo_[i] = 0; }
    }
    pboReady_ = false;
    InitFBO(width, height);
}

// ============================================================================
// Mesh upload
// ============================================================================

OpenGLRenderBackend::MeshCacheEntry& OpenGLRenderBackend::UploadMesh(Mesh* mesh) {
    uintptr_t key = reinterpret_cast<uintptr_t>(mesh);

    // Fast path: skip rebuild + re-upload if mesh data unchanged
    uint32_t currentVersion = mesh->GetGPUVersion();
    auto it = meshCache_.find(key);
    if (it != meshCache_.end() && it->second.lastVersion == currentVersion) {
        return it->second;
    }

    ITriangleGroup* triGroup = mesh->GetTriangleGroup();
    uint32_t triCount = triGroup->GetTriangleCount();
    Triangle3D* triangles = triGroup->GetTriangles();

    bool hasUV = mesh->HasUV();
    const Vector2D* uvVerts = hasUV ? mesh->GetUVVertices() : nullptr;
    const IndexGroup* uvIndices = hasUV ? mesh->GetUVIndexGroup() : nullptr;

    // Build interleaved vertex data (3 verts per triangle, flat shading)
    static std::vector<GLVertex> verts;
    verts.clear();
    verts.reserve(triCount * 3);

    for (uint32_t i = 0; i < triCount; ++i) {
        const Triangle3D& tri = triangles[i];
        const Vector3D& p1 = *tri.p1;
        const Vector3D& p2 = *tri.p2;
        const Vector3D& p3 = *tri.p3;

        // Face normal from cross product of edges
        float e1x = p2.X - p1.X, e1y = p2.Y - p1.Y, e1z = p2.Z - p1.Z;
        float e2x = p3.X - p1.X, e2y = p3.Y - p1.Y, e2z = p3.Z - p1.Z;
        float nx = e1y * e2z - e1z * e2y;
        float ny = e1z * e2x - e1x * e2z;
        float nz = e1x * e2y - e1y * e2x;
        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len > 1e-8f) {
            float inv = 1.0f / len;
            nx *= inv; ny *= inv; nz *= inv;
        }

        float u0 = 0, v0 = 0, u1 = 0, v1 = 0, u2 = 0, v2 = 0;
        if (hasUV && uvIndices && uvVerts) {
            const IndexGroup& uvIdx = uvIndices[i];
            u0 = uvVerts[uvIdx.A].X; v0 = uvVerts[uvIdx.A].Y;
            u1 = uvVerts[uvIdx.B].X; v1 = uvVerts[uvIdx.B].Y;
            u2 = uvVerts[uvIdx.C].X; v2 = uvVerts[uvIdx.C].Y;
        }

        verts.push_back({p1.X, p1.Y, p1.Z, nx, ny, nz, u0, v0});
        verts.push_back({p2.X, p2.Y, p2.Z, nx, ny, nz, u1, v1});
        verts.push_back({p3.X, p3.Y, p3.Z, nx, ny, nz, u2, v2});
    }

    if (it != meshCache_.end()) {
        // Re-upload vertex data for animated meshes
        MeshCacheEntry& entry = it->second;
        GLsizeiptr dataSize = static_cast<GLsizeiptr>(verts.size() * sizeof(GLVertex));
        glBindBuffer(GL_ARRAY_BUFFER, entry.vbo);
        if (entry.vertexCount == static_cast<int>(verts.size())) {
            glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, verts.data());
        } else {
            glBufferData(GL_ARRAY_BUFFER, dataSize, verts.data(), GL_DYNAMIC_DRAW);
            entry.vertexCount = static_cast<int>(verts.size());
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        entry.lastVersion = currentVersion;
        return entry;
    }

    MeshCacheEntry entry;
    entry.vertexCount = static_cast<int>(verts.size());
    entry.lastVersion = currentVersion;

    glGenVertexArrays(1, &entry.vao);
    glGenBuffers(1, &entry.vbo);

    glBindVertexArray(entry.vao);
    glBindBuffer(GL_ARRAY_BUFFER, entry.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(GLVertex)),
                 verts.data(), GL_DYNAMIC_DRAW);

    // position: location 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertex),
                          reinterpret_cast<void*>(offsetof(GLVertex, px)));
    glEnableVertexAttribArray(0);

    // normal: location 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertex),
                          reinterpret_cast<void*>(offsetof(GLVertex, nx)));
    glEnableVertexAttribArray(1);

    // uv: location 2
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex),
                          reinterpret_cast<void*>(offsetof(GLVertex, u)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    auto result = meshCache_.emplace(key, entry);
    return result.first->second;
}

void OpenGLRenderBackend::CleanupMeshCache() {
    for (auto& pair : meshCache_) {
        if (pair.second.vao) glDeleteVertexArrays(1, &pair.second.vao);
        if (pair.second.vbo) glDeleteBuffers(1, &pair.second.vbo);
    }
    meshCache_.clear();
}

// ============================================================================
// Sky rendering
// ============================================================================

void OpenGLRenderBackend::RenderSky(CameraBase* camera, int vpW, int vpH) {
    Sky& sky = Sky::GetInstance();
    bool hasSky = sky.IsEnabled() || camera->HasSkyGradient();
    if (!hasSky) return;

    // Sky rendering requires a KSL sky material
    KSLMaterial* kmat = sky.GetMaterial();
    if (!kmat || !kmat->IsBound() || !kmat->GetModule()->HasGPU()) return;

    glDisable(GL_DEPTH_TEST);

    GLuint prog = kmat->GetModule()->GetGLProgram();
    glUseProgram(prog);
    // Set u_shaderID for uber-shader dispatch
    if (kmat->GetModule()->IsUber()) {
        GLint sidLoc = glGetUniformLocation(prog, "u_shaderID");
        if (sidLoc >= 0)
            glUniform1i(sidLoc, kmat->GetModule()->GetUberShaderID());
    }
    // Set identity model/view/projection so the sky quad stays at NDC
    float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_model"), 1, GL_FALSE, identity);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_view"), 1, GL_FALSE, identity);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_projection"), 1, GL_FALSE, identity);
    BindKSLMaterialUniforms(prog, *kmat, *this);
    BindKSLTextures(prog, *kmat);

    glBindVertexArray(skyVao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

// ============================================================================
// Main render
// ============================================================================

void OpenGLRenderBackend::Render(Scene* scene, CameraBase* camera) {
    KL_PERF_SCOPE("GPU.Total");
    if (!initialized_ || !scene || !camera || camera->Is2D()) return;

    // Determine viewport size
    Vector2D minCoord = camera->GetCameraMinCoordinate();
    Vector2D maxCoord = camera->GetCameraMaxCoordinate();
    int vpW = static_cast<int>(maxCoord.X - minCoord.X + 1);
    int vpH = static_cast<int>(maxCoord.Y - minCoord.Y + 1);
    if (vpW <= 0 || vpH <= 0) return;

    // Create or resize FBO
    if (!fbo_ || fbWidth_ != vpW || fbHeight_ != vpH) {
        ResizeFBO(vpW, vpH);
    }

    // Bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, vpW, vpH);

    // Clear
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render sky gradient
    { KL_PERF_SCOPE("GPU.Sky");
    RenderSky(camera, vpW, vpH);
    }

    // Setup depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Backface culling
    if (camera->GetBackfaceCulling()) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    } else {
        glDisable(GL_CULL_FACE);
    }

    // Build view matrix
    camera->GetTransform()->SetBaseRotation(camera->GetCameraLayout()->GetRotation());
    Quaternion lookDir = camera->GetTransform()->GetRotation().Multiply(camera->GetLookOffset());
    Vector3D camPos = camera->GetTransform()->GetPosition();

    // Use LookAt with the camera's forward direction
    // Camera convention: forward is -Z in local space (matching software rasterizer)
    Vector3D forward = lookDir.RotateVector(Vector3D(0, 0, -1));
    Vector3D up = lookDir.RotateVector(Vector3D(0, 1, 0));
    Matrix4x4 viewMat = Matrix4x4::LookAt(camPos, camPos + forward, up);

    // Build projection matrix
    float aspect = static_cast<float>(vpW) / static_cast<float>(vpH);
    Matrix4x4 projMat;
    if (camera->IsPerspective()) {
        float fovRad = camera->GetFOV() * 3.14159265f / 180.0f;
        projMat = Matrix4x4::Perspective(fovRad, aspect,
                                          camera->GetNearPlane(), camera->GetFarPlane());
    } else {
        float halfW = static_cast<float>(vpW) * 0.5f;
        float halfH = static_cast<float>(vpH) * 0.5f;
        projMat = Matrix4x4::Orthographic(-halfW, halfW, -halfH, halfH,
                                           camera->GetNearPlane(), camera->GetFarPlane());
    }

    // Build view/projection matrices (used per-mesh when setting shader uniforms)
    // Pre-transpose once (row-major -> GL column-major)
    Matrix4x4 viewT = viewMat.Transpose();
    Matrix4x4 projT = projMat.Transpose();

    // Render each mesh
    { KL_PERF_SCOPE("GPU.Meshes");
    for (unsigned int i = 0; i < scene->GetMeshCount(); ++i) {
        Mesh* mesh = scene->GetMeshes()[i];
        if (!mesh || !mesh->IsEnabled()) continue;

        ITriangleGroup* triGroup = mesh->GetTriangleGroup();
        if (!triGroup || triGroup->GetTriangleCount() == 0) continue;

        IMaterial* material = mesh->GetMaterial();
        if (!material) continue;

        // Upload or retrieve cached mesh
        MeshCacheEntry& entry = UploadMesh(mesh);

        // Vertices are already in world space (Mesh::UpdateTransform bakes TRS
        // into the vertex buffer), so the model matrix is identity.
        Matrix4x4 modelMat;

        // Look up shader - all materials should be KSLMaterial
        GLuint prog;
        const KSLMaterial* kmat = static_cast<const KSLMaterial*>(material->IsKSL() ? material : nullptr);
        if (kmat && kmat->IsBound() && kmat->GetModule()->HasGPU()) {
            // KSLMaterial: use module's GL program + auto-bind params
            prog = kmat->GetModule()->GetGLProgram();
            glUseProgram(prog);
            const auto& u = GetUniforms(prog);

            glUniformMatrix4fv(u.view, 1, GL_FALSE, &viewT.M[0][0]);
            glUniformMatrix4fv(u.projection, 1, GL_FALSE, &projT.M[0][0]);
            if (u.time >= 0)
                glUniform1f(u.time, kmat->GetTime());

            BindKSLMaterialUniforms(prog, *kmat, *this);
            BindKSLTextures(prog, *kmat);
        } else {
            // Fallback: pink error shader
            prog = pinkProgram_;
            glUseProgram(prog);
            const auto& u = GetUniforms(prog);
            glUniformMatrix4fv(u.view, 1, GL_FALSE, &viewT.M[0][0]);
            glUniformMatrix4fv(u.projection, 1, GL_FALSE, &projT.M[0][0]);
        }

        // Set model matrix
        Matrix4x4 modelT = modelMat.Transpose();
        {
            const auto& u = GetUniforms(prog);
            glUniformMatrix4fv(u.model, 1, GL_FALSE, &modelT.M[0][0]);
        }

        // Draw
        glBindVertexArray(entry.vao);
        glDrawArrays(GL_TRIANGLES, 0, entry.vertexCount);
        glBindVertexArray(0);
    }
    } // end GPU.Meshes scope

    // Restore GL state so display backend's Present() isn't affected
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glUseProgram(0);

    // --- PBO async readback: kick DMA transfer for this frame ---
    {
        int bufSize = fbWidth_ * fbHeight_ * 3;
        // Lazy-create PBOs on first use or resize
        if (!pbo_[0] || !pbo_[1]) {
            glGenBuffers(2, pbo_);
            for (int i = 0; i < 2; ++i) {
                glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_[i]);
                glBufferData(GL_PIXEL_PACK_BUFFER, bufSize, nullptr, GL_STREAM_READ);
            }
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            pboIndex_ = 0;
            pboReady_ = false;
        }

        // Start async read from FBO into current PBO (non-blocking)
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_[pboIndex_]);
        glReadPixels(0, 0, fbWidth_, fbHeight_, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }

    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderBackend::RenderDirect(Scene* scene, CameraBase* camera) {
    KL_PERF_SCOPE("GPU.Total");
    if (!initialized_ || !scene || !camera || camera->Is2D()) return;

    Vector2D minCoord = camera->GetCameraMinCoordinate();
    Vector2D maxCoord = camera->GetCameraMaxCoordinate();
    int vpW = static_cast<int>(maxCoord.X - minCoord.X + 1);
    int vpH = static_cast<int>(maxCoord.Y - minCoord.Y + 1);
    if (vpW <= 0 || vpH <= 0) return;

    if (!fbo_ || fbWidth_ != vpW || fbHeight_ != vpH) {
        ResizeFBO(vpW, vpH);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, vpW, vpH);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    { KL_PERF_SCOPE("GPU.Sky");
    RenderSky(camera, vpW, vpH);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    if (camera->GetBackfaceCulling()) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    } else {
        glDisable(GL_CULL_FACE);
    }

    camera->GetTransform()->SetBaseRotation(camera->GetCameraLayout()->GetRotation());
    Quaternion lookDir = camera->GetTransform()->GetRotation().Multiply(camera->GetLookOffset());
    Vector3D camPos = camera->GetTransform()->GetPosition();
    Vector3D forward = lookDir.RotateVector(Vector3D(0, 0, -1));
    Vector3D up = lookDir.RotateVector(Vector3D(0, 1, 0));
    Matrix4x4 viewMat = Matrix4x4::LookAt(camPos, camPos + forward, up);

    float aspect = static_cast<float>(vpW) / static_cast<float>(vpH);
    Matrix4x4 projMat;
    if (camera->IsPerspective()) {
        float fovRad = camera->GetFOV() * 3.14159265f / 180.0f;
        projMat = Matrix4x4::Perspective(fovRad, aspect,
                                          camera->GetNearPlane(), camera->GetFarPlane());
    } else {
        float halfW = static_cast<float>(vpW) * 0.5f;
        float halfH = static_cast<float>(vpH) * 0.5f;
        projMat = Matrix4x4::Orthographic(-halfW, halfW, -halfH, halfH,
                                           camera->GetNearPlane(), camera->GetFarPlane());
    }

    Matrix4x4 viewT = viewMat.Transpose();
    Matrix4x4 projT = projMat.Transpose();

    { KL_PERF_SCOPE("GPU.Meshes");

    // Build sortable mesh list by material to minimize state changes
    unsigned int meshCount = scene->GetMeshCount();
    Mesh** meshes = scene->GetMeshes();
    const bool useUber = kslRegistry_.HasUberShader();

    struct MeshSortEntry {
        unsigned int index;
        GLuint prog;
        uintptr_t matKey;
    };
    static std::vector<MeshSortEntry> sortedMeshes;
    sortedMeshes.clear();
    sortedMeshes.reserve(meshCount);

    for (unsigned int i = 0; i < meshCount; ++i) {
        Mesh* mesh = meshes[i];
        if (!mesh || !mesh->IsEnabled()) continue;
        ITriangleGroup* triGroup = mesh->GetTriangleGroup();
        if (!triGroup || triGroup->GetTriangleCount() == 0) continue;
        IMaterial* material = mesh->GetMaterial();
        if (!material) continue;

        GLuint prog = pinkProgram_;
        const KSLMaterial* kmat = static_cast<const KSLMaterial*>(material->IsKSL() ? material : nullptr);
        if (kmat && kmat->IsBound() && kmat->GetModule()->HasGPU()) {
            prog = kmat->GetModule()->GetGLProgram();
        }
        sortedMeshes.push_back({i, prog, reinterpret_cast<uintptr_t>(material)});
    }

    // Sort by (program, material) to batch same-material meshes
    std::sort(sortedMeshes.begin(), sortedMeshes.end(),
        [](const MeshSortEntry& a, const MeshSortEntry& b) {
            if (a.prog != b.prog) return a.prog < b.prog;
            return a.matKey < b.matKey;
        });

    Matrix4x4 identityModelT;
    GLuint lastProg = 0;
    uintptr_t lastMat = 0;

    // Uber-shader: bind once, set matrices once
    if (useUber) {
        GLuint uberProg = kslRegistry_.GetUberProgram();
        glUseProgram(uberProg);
        const auto& u = GetUniforms(uberProg);
        glUniformMatrix4fv(u.view, 1, GL_FALSE, &viewT.M[0][0]);
        glUniformMatrix4fv(u.projection, 1, GL_FALSE, &projT.M[0][0]);
        glUniformMatrix4fv(u.model, 1, GL_FALSE, &identityModelT.M[0][0]);
        lastProg = uberProg;
    }

    // Collect same-material meshes for merged VBO draw
    struct BatchEntry { Mesh* mesh; };
    static std::vector<BatchEntry> currentBatch;
    currentBatch.clear();
    currentBatch.reserve(64);
    static std::vector<GLVertex> s_batchVerts;
    s_batchVerts.reserve(1024);

    auto flushBatch = [&]() {
        if (currentBatch.empty()) return;
        if (currentBatch.size() == 1) {
            // Single mesh - use cached per-mesh VAO (zero CPU vertex rebuild)
            MeshCacheEntry& entry = UploadMesh(currentBatch[0].mesh);
            glBindVertexArray(entry.vao);
            glDrawArrays(GL_TRIANGLES, 0, entry.vertexCount);
        } else {
            // Multiple meshes - merge into batch VBO for single draw call
            s_batchVerts.clear();
            for (const auto& be : currentBatch) {
                Mesh* m = be.mesh;
                ITriangleGroup* tg = m->GetTriangleGroup();
                uint32_t tc = tg->GetTriangleCount();
                Triangle3D* tris = tg->GetTriangles();
                bool hasUV = m->HasUV();
                const Vector2D* uvV = hasUV ? m->GetUVVertices() : nullptr;
                const IndexGroup* uvI = hasUV ? m->GetUVIndexGroup() : nullptr;

                for (uint32_t t = 0; t < tc; ++t) {
                    const Triangle3D& tri = tris[t];
                    const Vector3D& p1 = *tri.p1;
                    const Vector3D& p2 = *tri.p2;
                    const Vector3D& p3 = *tri.p3;
                    float nx = (p2.Y-p1.Y)*(p3.Z-p1.Z) - (p2.Z-p1.Z)*(p3.Y-p1.Y);
                    float ny = (p2.Z-p1.Z)*(p3.X-p1.X) - (p2.X-p1.X)*(p3.Z-p1.Z);
                    float nz = (p2.X-p1.X)*(p3.Y-p1.Y) - (p2.Y-p1.Y)*(p3.X-p1.X);
                    float len = std::sqrt(nx*nx + ny*ny + nz*nz);
                    if (len > 1e-8f) { float inv = 1.0f/len; nx *= inv; ny *= inv; nz *= inv; }
                    float u0=0,v0=0,u1=0,v1=0,u2=0,v2=0;
                    if (hasUV && uvI && uvV) {
                        const IndexGroup& ui = uvI[t];
                        u0=uvV[ui.A].X; v0=uvV[ui.A].Y;
                        u1=uvV[ui.B].X; v1=uvV[ui.B].Y;
                        u2=uvV[ui.C].X; v2=uvV[ui.C].Y;
                    }
                    s_batchVerts.push_back({p1.X,p1.Y,p1.Z, nx,ny,nz, u0,v0});
                    s_batchVerts.push_back({p2.X,p2.Y,p2.Z, nx,ny,nz, u1,v1});
                    s_batchVerts.push_back({p3.X,p3.Y,p3.Z, nx,ny,nz, u2,v2});
                }
            }
            glBindVertexArray(batchVao_);
            glBindBuffer(GL_ARRAY_BUFFER, batchVbo_);
            GLsizeiptr sz = static_cast<GLsizeiptr>(s_batchVerts.size() * sizeof(GLVertex));
            if (sz > batchVboSize_) {
                glBufferData(GL_ARRAY_BUFFER, sz, s_batchVerts.data(), GL_STREAM_DRAW);
                batchVboSize_ = sz;
            } else {
                glBufferSubData(GL_ARRAY_BUFFER, 0, sz, s_batchVerts.data());
            }
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(s_batchVerts.size()));
        }
        currentBatch.clear();
    };

    for (const auto& entry : sortedMeshes) {
        Mesh* mesh = meshes[entry.index];
        IMaterial* material = mesh->GetMaterial();
        const KSLMaterial* kmat = static_cast<const KSLMaterial*>(material->IsKSL() ? material : nullptr);
        GLuint prog = entry.prog;

        // Program change (skipped in uber mode - all share same program)
        if (!useUber && prog != lastProg) {
            flushBatch();
            glUseProgram(prog);
            const auto& u = GetUniforms(prog);
            glUniformMatrix4fv(u.view, 1, GL_FALSE, &viewT.M[0][0]);
            glUniformMatrix4fv(u.projection, 1, GL_FALSE, &projT.M[0][0]);
            glUniformMatrix4fv(u.model, 1, GL_FALSE, &identityModelT.M[0][0]);
            lastProg = prog;
            lastMat = 0;
        }

        if (entry.matKey != lastMat) {
            flushBatch();
            if (kmat && prog != pinkProgram_) {
                GLuint activeProg = useUber ? kslRegistry_.GetUberProgram() : prog;
                const auto& u = GetUniforms(activeProg);
                if (u.time >= 0)
                    glUniform1f(u.time, kmat->GetTime());
                // Set u_shaderID for uber-shader dispatch
                if (useUber && kmat->GetModule()->IsUber()) {
                    GLint sidLoc = GetKSLUniform(activeProg, "u_shaderID");
                    if (sidLoc >= 0)
                        glUniform1i(sidLoc, kmat->GetModule()->GetUberShaderID());
                }
                BindKSLMaterialUniforms(activeProg, *kmat, *this);
                BindKSLTextures(activeProg, *kmat);
            }
            lastMat = entry.matKey;
        }

        currentBatch.push_back({mesh});
    }
    flushBatch();
    } // end GPU.Meshes scope

    // Render debug lines (grids, axes, rays)
    RenderDebugLines(&viewT.M[0][0], &projT.M[0][0]);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glUseProgram(0);

    // No PBO readback - FBO stays on GPU for BlitToScreen()
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderBackend::ReadPixels(Color888* buffer, int width, int height) {
    KL_PERF_SCOPE("GPU.ReadPixels");
    if (!initialized_ || !buffer || !fbo_) return;

    // On the first frame, PBO readback isn't ready yet - fall back to sync
    if (!pboReady_) {
        pboReady_ = true;
        pboIndex_ = 1 - pboIndex_;  // swap so next Render writes to the other PBO

        int total = width * height;
        int rowBytes = width * 3;
        readbackBuf_.resize(total * 3);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, readbackBuf_.data());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        for (int y = 0; y < height; ++y) {
            int srcRow = (height - 1 - y) * rowBytes;
            std::memcpy(&buffer[y * width], &readbackBuf_[srcRow], rowBytes);
        }
        return;
    }

    // Map the *previous* frame's PBO (already finished DMA transfer)
    int readPbo = 1 - pboIndex_;
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_[readPbo]);
    const auto* src = static_cast<const uint8_t*>(
        glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));

    if (src) {
        int rowBytes = width * 3;
        for (int y = 0; y < height; ++y) {
            int srcRow = (height - 1 - y) * rowBytes;
            std::memcpy(&buffer[y * width], &src[srcRow], rowBytes);
        }
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    // Swap PBO index for next frame
    pboIndex_ = 1 - pboIndex_;
}

void OpenGLRenderBackend::BlitToScreen(int screenW, int screenH) {
    if (!initialized_ || !fbo_) return;

    // Draw the FBO color texture as a fullscreen quad on the default
    // framebuffer.  This replaces glBlitFramebuffer which does not
    // reliably produce visible output on Wayland + llvmpipe/Mesa.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenW, screenH);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glUseProgram(overlayProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTex_);
    GLint texLoc = glGetUniformLocation(overlayProgram_, "u_texture");
    glUniform1i(texLoc, 0);

    glBindVertexArray(blitVao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLRenderBackend::RenderDebugLines(const float* viewMat, const float* projMat) {
    auto& dd = DebugDraw::GetInstance();
    if (!dd.IsEnabled()) return;
    const auto& lines = dd.GetLines();
    if (lines.empty()) return;

    // Build vertex data: 2 vertices per line, 7 floats each (pos3 + color4)
    static std::vector<float> verts;
    verts.clear();
    verts.reserve(lines.size() * 2 * 7);
    // Separate depth-tested and non-depth-tested lines
    size_t depthTestedCount = 0;
    // First pass: depth-tested lines
    for (const auto& l : lines) {
        if (!l.depthTest) continue;
        verts.push_back(l.start.X); verts.push_back(l.start.Y); verts.push_back(l.start.Z);
        verts.push_back(l.color.r); verts.push_back(l.color.g); verts.push_back(l.color.b); verts.push_back(l.color.a);
        verts.push_back(l.end.X); verts.push_back(l.end.Y); verts.push_back(l.end.Z);
        verts.push_back(l.color.r); verts.push_back(l.color.g); verts.push_back(l.color.b); verts.push_back(l.color.a);
        depthTestedCount += 2;
    }
    // Second pass: non-depth-tested lines (drawn on top)
    size_t noDepthCount = 0;
    for (const auto& l : lines) {
        if (l.depthTest) continue;
        verts.push_back(l.start.X); verts.push_back(l.start.Y); verts.push_back(l.start.Z);
        verts.push_back(l.color.r); verts.push_back(l.color.g); verts.push_back(l.color.b); verts.push_back(l.color.a);
        verts.push_back(l.end.X); verts.push_back(l.end.Y); verts.push_back(l.end.Z);
        verts.push_back(l.color.r); verts.push_back(l.color.g); verts.push_back(l.color.b); verts.push_back(l.color.a);
        noDepthCount += 2;
    }
    if (verts.empty()) return;

    glUseProgram(lineProgram_);
    GLint viewLoc = glGetUniformLocation(lineProgram_, "u_view");
    GLint projLoc = glGetUniformLocation(lineProgram_, "u_projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMat);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMat);

    glBindVertexArray(lineVao_);
    glBindBuffer(GL_ARRAY_BUFFER, lineVbo_);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STREAM_DRAW);

    // Draw depth-tested lines
    if (depthTestedCount > 0) {
        glEnable(GL_DEPTH_TEST);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(depthTestedCount));
    }
    // Draw non-depth-tested lines on top
    if (noDepthCount > 0) {
        glDisable(GL_DEPTH_TEST);
        glDrawArrays(GL_LINES, static_cast<GLint>(depthTestedCount), static_cast<GLsizei>(noDepthCount));
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void OpenGLRenderBackend::CompositeCanvasOverlays(int screenW, int screenH) {
    auto& canvases = Canvas2D::ActiveList();
    if (canvases.empty()) return;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenW, screenH);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(overlayProgram_);
    GLint texLoc = glGetUniformLocation(overlayProgram_, "u_texture");
    glUniform1i(texLoc, 0);
    glActiveTexture(GL_TEXTURE0);

    for (auto* canvas : canvases) {
        int cw = canvas->GetWidth();
        int ch = canvas->GetHeight();
        if (cw <= 0 || ch <= 0 || !canvas->IsDirty()) continue;

        const Color888* pixels = canvas->GetPixels();
        const uint8_t* alpha = canvas->GetAlpha();
        size_t totalPixels = static_cast<size_t>(cw) * ch;

        // Resize persistent RGBA buffer if needed (cleared to transparent)
        if (overlayRgba_.size() != totalPixels * 4) {
            overlayRgba_.assign(totalPixels * 4, 0);
        }

        // Only convert dirty region to RGBA
        int dx0 = canvas->GetDirtyMinX();
        int dy0 = canvas->GetDirtyMinY();
        int dx1 = canvas->GetDirtyMaxX();
        int dy1 = canvas->GetDirtyMaxY();

        for (int y = dy0; y <= dy1; ++y) {
            int rowOff = y * cw;
            for (int x = dx0; x <= dx1; ++x) {
                int si = rowOff + x;
                int di = si * 4;
                overlayRgba_[di + 0] = pixels[si].R;
                overlayRgba_[di + 1] = pixels[si].G;
                overlayRgba_[di + 2] = pixels[si].B;
                overlayRgba_[di + 3] = alpha[si];
            }
        }

        // Create/resize texture as needed
        if (!overlayTex_ || overlayTexW_ != cw || overlayTexH_ != ch) {
            if (overlayTex_) glDeleteTextures(1, &overlayTex_);
            glGenTextures(1, &overlayTex_);
            glBindTexture(GL_TEXTURE_2D, overlayTex_);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            overlayTexW_ = cw;
            overlayTexH_ = ch;
            // Initialize full texture as transparent
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, cw, ch, 0, GL_RGBA, GL_UNSIGNED_BYTE, overlayRgba_.data());
        } else {
            glBindTexture(GL_TEXTURE_2D, overlayTex_);
        }

        // Upload only the dirty rows via sub-image
        int rh = dy1 - dy0 + 1;
        glPixelStorei(GL_UNPACK_ROW_LENGTH, cw);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, dy0, cw, rh,
                        GL_RGBA, GL_UNSIGNED_BYTE,
                        &overlayRgba_[dy0 * cw * 4]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        glBindVertexArray(overlayVao_);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // Clear the dirty pixels in our persistent buffer (reset to transparent)
        for (int y = dy0; y <= dy1; ++y) {
            std::memset(&overlayRgba_[(y * cw + dx0) * 4], 0, (dx1 - dx0 + 1) * 4);
        }

        canvas->Clear();
    }

    glDisable(GL_BLEND);
    glUseProgram(0);
}

} // namespace koilo
