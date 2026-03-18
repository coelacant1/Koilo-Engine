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
    bool HasGPU() const {
#ifdef KL_HAVE_OPENGL_BACKEND
        return glProgram_ != 0;
#else
        return false;
#endif
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
        return shadeFn_ != nullptr;
    }

    // Load .glsl file and compile to GL program
    bool LoadGLSL(const std::string& path, const std::string& vertexSrc) {
#ifdef KL_HAVE_OPENGL_BACKEND
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::stringstream ss;
        ss << file.rdbuf();
        std::string fragSrc = ss.str();

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
        return false;
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

    ParamList GetParams() const {
        if (paramsFn_) {
            int count = 0;
            const ParamDecl* decls = paramsFn_(&count);
            return {decls, count};
        }
        return {nullptr, 0};
    }

#ifdef KL_HAVE_OPENGL_BACKEND
    GLuint GetGLProgram() const { return glProgram_; }
    GLint GetUniformView() const { return uView_; }
    GLint GetUniformProjection() const { return uProjection_; }
    GLint GetUniformModel() const { return uModel_; }
    GLint GetUniformCameraPos() const { return uCameraPos_; }
    GLint GetUniformTime() const { return uTime_; }

    // Uber-shader: share a single GL program across all modules
    void SetUberProgram(GLuint prog, int shaderID) {
        glProgram_ = prog;
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
    bool IsUber() const { return isUber_; }
    int GetUberShaderID() const { return uberShaderID_; }
#endif

    void Unload() {
#ifdef KL_HAVE_OPENGL_BACKEND
        if (glProgram_ && !isUber_) { glDeleteProgram(glProgram_); }
        glProgram_ = 0;
        isUber_ = false;
        uberShaderID_ = -1;
#endif
        elfLoader_.Unload();
        shadeFn_ = nullptr;
        infoFn_ = nullptr;
        createFn_ = nullptr;
        destroyFn_ = nullptr;
        setParamFn_ = nullptr;
        paramsFn_ = nullptr;
        requiredAttribs_ = SHADE_ATTRIB_ALL;
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

    // GPU (GL program)
#ifdef KL_HAVE_OPENGL_BACKEND
    GLuint glProgram_ = 0;
    GLint uView_ = -1;
    GLint uProjection_ = -1;
    GLint uModel_ = -1;
    GLint uCameraPos_ = -1;
    GLint uTime_ = -1;
    bool isUber_ = false;
    int uberShaderID_ = -1;
#endif
};

} // namespace ksl

