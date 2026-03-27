// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ksl_shader.hpp"
#include "ksl_elf_loader.hpp"
#include <koilo/kernel/logging/log.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>

#ifdef KL_HAVE_OPENGL_BACKEND
#include <glad/glad.h>
#endif

namespace ksl {

// Represents one loaded KSL shader: .kso (CPU via ELF) + .glsl (GPU)
class KSLModule {
public:
    KSLModule() = default;
    ~KSLModule() { Unload(); }

    KSLModule(const KSLModule&) = delete;
    KSLModule& operator=(const KSLModule&) = delete;

    const std::string& Name() const { return name_; }
    bool HasCPU() const { return shadeFn_ != nullptr; }
    bool HasGPU() const { return glProgram_ != 0; }

    /// True if GLSL source is available for RHI shader creation.
    bool HasGLSLSource() const { return !glslFragSource_.empty(); }

    /// True if the module can provide GPU shading (GL program or GLSL source).
    bool HasShaderData() const { return HasGPU() || HasGLSLSource(); }

    /// Retained GLSL fragment shader source (available on all platforms).
    const std::string& GetGLSLFragmentSource() const { return glslFragSource_; }

    /// Retained GLSL vertex shader source (available on all platforms).
    const std::string& GetGLSLVertexSource() const { return glslVertSource_; }

    /// Retain GLSL source strings without compiling a GL program.
    /// Used when the uber-shader handles legacy GL rendering but
    /// per-module source is still needed for RHI shader creation.
    bool RetainGLSLSource(const std::string& path, const std::string& vertexSrc) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        std::stringstream ss;
        ss << file.rdbuf();
        glslFragSource_ = ss.str();
        glslVertSource_ = vertexSrc;
        return !glslFragSource_.empty();
    }

    // Load .kso (ELF) for CPU shading - unified across all platforms
    bool LoadKSO(const std::string& path, const KSLSymbolTable& symbols) {
        // Read file into memory
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;

        auto size = file.tellg();
        if (size <= 0) return false;
        file.seekg(0);

        std::vector<uint8_t> data(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(data.data()), size);
        if (!file) return false;

        // Load via ELF loader
        if (!elfLoader_.Load(data.data(), data.size(), symbols)) {
            KL_ERR("KSL", "Failed to load %s: %s",
                    path.c_str(), elfLoader_.GetError());
            return false;
        }

        // Resolve function pointers from exports
        infoFn_     = elfLoader_.GetSymbol<KSLInfoFn>("ksl_info");
        createFn_   = elfLoader_.GetSymbol<KSLCreateFn>("ksl_create");
        destroyFn_  = elfLoader_.GetSymbol<KSLDestroyFn>("ksl_destroy");
        setParamFn_ = elfLoader_.GetSymbol<KSLSetParamFn>("ksl_set_param");
        shadeFn_    = elfLoader_.GetSymbol<KSLShadeFn>("ksl_shade");
        paramsFn_   = elfLoader_.GetSymbol<KSLParamsFn>("ksl_params");

        if (infoFn_) {
            const KSLShaderInfo* info = infoFn_();
            if (info) {
                name_ = info->name;
                requiredAttribs_ = info->requiredAttribs;
            }
        }

        // Resolve optional metadata (render hints etc.)
        auto metaFn = elfLoader_.GetSymbol<KSLMetadataFn>("ksl_metadata");
        if (metaFn) {
            int count = 0;
            const MetaEntry* entries = metaFn(&count);
            if (entries && count > 0) {
                metadata_.assign(entries, entries + count);
            }
        }

        return shadeFn_ != nullptr;
    }

    // Load .glsl file and compile to GL program.
    // Also retains GLSL source strings for RHI shader creation (Phase 17f).
    bool LoadGLSL(const std::string& path, const std::string& vertexSrc) {
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::stringstream ss;
        ss << file.rdbuf();
        std::string fragSrc = ss.str();

        // Retain GLSL source for backend-agnostic shader creation via RHI
        glslFragSource_ = fragSrc;
        glslVertSource_ = vertexSrc;

#ifdef KL_HAVE_OPENGL_BACKEND

        GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexSrc.c_str());
        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragSrc.c_str());
        if (!vs || !fs) {
            if (vs) glDeleteShader(vs);
            if (fs) glDeleteShader(fs);
            return false;
        }

        glProgram_ = glCreateProgram();
        glAttachShader(glProgram_, vs);
        glAttachShader(glProgram_, fs);
        glLinkProgram(glProgram_);

        GLint linked;
        glGetProgramiv(glProgram_, GL_LINK_STATUS, &linked);
        glDeleteShader(vs);
        glDeleteShader(fs);

        if (!linked) {
            char log[1024];
            glGetProgramInfoLog(glProgram_, sizeof(log), nullptr, log);
            KL_ERR("KSL", "Shader link error: %s", log);
            glDeleteProgram(glProgram_);
            glProgram_ = 0;
            return false;
        }

        // Cache standard uniform locations
        uView_       = glGetUniformLocation(glProgram_, "u_view");
        uProjection_ = glGetUniformLocation(glProgram_, "u_projection");
        uModel_      = glGetUniformLocation(glProgram_, "u_model");
        uCameraPos_  = glGetUniformLocation(glProgram_, "u_cameraPos");
        uTime_       = glGetUniformLocation(glProgram_, "u_time");

        return true;
#else
        // No GL context available - GLSL source is retained for RHI
        // shader creation but no GL program is compiled.
        return !glslFragSource_.empty();
#endif
    }

    // CPU shade call
    void* CreateInstance() const { return createFn_ ? createFn_() : nullptr; }
    void DestroyInstance(void* inst) const { if (destroyFn_) destroyFn_(inst); }
    void SetParam(void* inst, const char* name, const void* data, int type, int count) const {
        if (setParamFn_) setParamFn_(inst, name, data, type, count);
    }
    vec4 Shade(void* inst, const ShadeInput* input) const {
        if (shadeFn_) return shadeFn_(inst, input);
        return vec4(1.0f, 0.0f, 0.78f, 1.0f); // pink error
    }

    KSLShadeFn GetShadeFn() const { return shadeFn_; }

    uint8_t GetRequiredAttribs() const { return requiredAttribs_; }

    /// @brief Return the metadata list loaded from the .kso module.
    MetaList GetMetadata() const { return {metadata_.data(), static_cast<int>(metadata_.size())}; }

    /// @brief Look up an integer metadata value by key. Returns fallback if not found.
    int GetMetaInt(const char* key, int fallback = 0) const {
        for (const auto& e : metadata_) {
            if (e.type == MetaType::Int && std::strcmp(e.key, key) == 0)
                return e.intVal;
        }
        return fallback;
    }

    /// @brief Look up a float metadata value by key. Returns fallback if not found.
    float GetMetaFloat(const char* key, float fallback = 0.0f) const {
        for (const auto& e : metadata_) {
            if (e.type == MetaType::Float && std::strcmp(e.key, key) == 0)
                return e.floatVal;
        }
        return fallback;
    }

    ParamList GetParams() const {
        if (paramsFn_) {
            int count = 0;
            const ParamDecl* decls = paramsFn_(&count);
            return {decls, count};
        }
        return {nullptr, 0};
    }

#ifdef KL_HAVE_OPENGL_BACKEND
    GLuint GetGLProgram() const { return static_cast<GLuint>(glProgram_); }
    GLint GetUniformView() const { return static_cast<GLint>(uView_); }
    GLint GetUniformProjection() const { return static_cast<GLint>(uProjection_); }
    GLint GetUniformModel() const { return static_cast<GLint>(uModel_); }
    GLint GetUniformCameraPos() const { return static_cast<GLint>(uCameraPos_); }
    GLint GetUniformTime() const { return static_cast<GLint>(uTime_); }

    // Uber-shader: share a single GL program across all modules
    void SetUberProgram(GLuint prog, int shaderID) {
        glProgram_ = static_cast<unsigned int>(prog);
        uberShaderID_ = shaderID;
        isUber_ = true;
        // Cache standard uniform locations from uber program
        uView_       = glGetUniformLocation(prog, "u_view");
        uProjection_ = glGetUniformLocation(prog, "u_projection");
        uModel_      = glGetUniformLocation(prog, "u_model");
        uCameraPos_  = glGetUniformLocation(prog, "u_cameraPos");
        uTime_       = glGetUniformLocation(prog, "u_time");
    }

    void DetachGLProgram() { glProgram_ = 0; }
#endif
    bool IsUber() const { return isUber_; }
    int GetUberShaderID() const { return uberShaderID_; }

    void Unload() {
#ifdef KL_HAVE_OPENGL_BACKEND
        if (glProgram_ && !isUber_) { glDeleteProgram(glProgram_); }
#endif
        glProgram_ = 0;
        isUber_ = false;
        uberShaderID_ = -1;
        elfLoader_.Unload();
        shadeFn_ = nullptr;
        infoFn_ = nullptr;
        createFn_ = nullptr;
        destroyFn_ = nullptr;
        setParamFn_ = nullptr;
        paramsFn_ = nullptr;
        requiredAttribs_ = SHADE_ATTRIB_ALL;
        metadata_.clear();
        glslFragSource_.clear();
        glslVertSource_.clear();
    }

private:
#ifdef KL_HAVE_OPENGL_BACKEND
    static GLuint CompileShader(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(s, 512, nullptr, log);
            KL_ERR("KSL", "Shader compile error: %s", log);
            glDeleteShader(s);
            return 0;
        }
        return s;
    }
#endif

    std::string name_;

    // CPU shader: ELF loader (replaces dlopen/LoadLibrary)
    KSLElfLoader elfLoader_;

    KSLInfoFn     infoFn_ = nullptr;
    KSLCreateFn   createFn_ = nullptr;
    KSLDestroyFn  destroyFn_ = nullptr;
    KSLSetParamFn setParamFn_ = nullptr;
    KSLShadeFn    shadeFn_ = nullptr;
    KSLParamsFn   paramsFn_ = nullptr;
    uint8_t       requiredAttribs_ = SHADE_ATTRIB_ALL;

    // Shader metadata (render hints, etc.) loaded from .kso
    std::vector<MetaEntry> metadata_;

    // GLSL source retention for backend-agnostic RHI shader creation (Phase 17f)
    std::string glslFragSource_;
    std::string glslVertSource_;

    // GPU (GL program) - always present for consistent layout across TUs (ODR).
    unsigned int glProgram_ = 0;
    int uView_ = -1;
    int uProjection_ = -1;
    int uModel_ = -1;
    int uCameraPos_ = -1;
    int uTime_ = -1;
    bool isUber_ = false;
    int uberShaderID_ = -1;
};

} // namespace ksl

