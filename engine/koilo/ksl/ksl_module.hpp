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

namespace ksl {

/// Represents one loaded KSL shader: .kso (CPU via ELF) + .glsl source (GPU).
///
/// The module stores raw GLSL source strings for backend-agnostic shader
/// creation via the RHI device.  No OpenGL (or any other GPU API) headers
/// or calls are present -- all GPU compilation is deferred to the RHI driver.
class KSLModule {
public:
    KSLModule() = default;
    ~KSLModule() { Unload(); }

    KSLModule(const KSLModule&) = delete;
    KSLModule& operator=(const KSLModule&) = delete;

    const std::string& Name() const { return name_; }
    bool HasCPU() const { return shadeFn_ != nullptr; }

    /// True if GLSL source is available for RHI shader creation.
    bool HasGLSLSource() const { return !glslFragSource_.empty(); }

    /// True if the module can provide GPU shading (GLSL or SPIR-V source).
    bool HasShaderData() const { return HasGLSLSource(); }

    /// Retained GLSL fragment shader source (available on all platforms).
    const std::string& GetGLSLFragmentSource() const { return glslFragSource_; }

    /// Retained GLSL vertex shader source (available on all platforms).
    const std::string& GetGLSLVertexSource() const { return glslVertSource_; }

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

    /// Load a .glsl file and retain its source for RHI shader creation.
    ///
    /// No GPU API calls are made here -- the RHI device compiles the
    /// source when creating pipeline objects.
    bool LoadGLSL(const std::string& path, const std::string& vertexSrc) {
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::stringstream ss;
        ss << file.rdbuf();
        glslFragSource_ = ss.str();
        glslVertSource_ = vertexSrc;
        glslFilePath_   = path;

        return !glslFragSource_.empty();
    }

    /// Re-read the GLSL file from disk (hot-reload).
    /// Returns true if the source was updated successfully.
    bool ReloadGLSL() {
        if (glslFilePath_.empty()) return false;
        std::ifstream file(glslFilePath_);
        if (!file.is_open()) return false;
        std::stringstream ss;
        ss << file.rdbuf();
        std::string newSrc = ss.str();
        if (newSrc.empty()) return false;
        glslFragSource_ = std::move(newSrc);
        return true;
    }

    /// File path the GLSL fragment source was loaded from (empty if none).
    const std::string& GetGLSLFilePath() const { return glslFilePath_; }

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

    void Unload() {
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
        glslFilePath_.clear();
    }

private:
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

    // GLSL source retention for backend-agnostic RHI shader creation
    std::string glslFragSource_;
    std::string glslVertSource_;
    std::string glslFilePath_;   // Original file path for hot-reload
};

} // namespace ksl

