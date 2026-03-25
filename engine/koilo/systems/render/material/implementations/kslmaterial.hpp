// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file kslmaterial.hpp
 * @brief Bridge material that wraps a KSLModule into the IMaterial/IShader system.
 *
 * KSLMaterial is the primary way to use KSL shaders from scripts and C++.
 * Supports both GPU (GLSL) and CPU (.kso) rendering paths. Includes light
 * management, camera position, and generic parameter setting.
 *
 * Script usage:
 *   var mat = KSLMaterial("pbr");
 *   mat.SetFloat("roughness", 0.5);
 *   mat.AddLight();
 *   mat.SetLightPosition(0, Vector3D(0, 10, 0));
 *   mesh.SetMaterial(mat);
 */

#pragma once

#include <koilo/systems/render/material/imaterial.hpp>
#include <koilo/systems/render/shader/ishader.hpp>
#include <koilo/core/color/color888.hpp>
#include <koilo/core/math/mathematics.hpp>
#include <koilo/ksl/ksl_module.hpp>
#include <koilo/ksl/ksl_registry.hpp>
#include <koilo/assets/image/texture.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

namespace koilo {

class KSLMaterial; // forward

/**
 * @class KSLShaderBridge
 * @brief IShader that delegates shading to a KSLModule.
 *
 * Populates ShadeInput with light data and camera info from the owning
 * KSLMaterial, then calls the module's CPU shade function.
 */
class KSLShaderBridge : public IShader {
public:
    void SetModule(ksl::KSLModule* mod) { module_ = mod; }
    void SetInstance(void* inst) { instance_ = inst; }
    void SetOwner(const KSLMaterial* owner) { owner_ = owner; }
    void SetTime(float time, float dt, int frame) {
        time_ = time; dt_ = dt; frame_ = frame;
        frameCtx_.time = time;
        frameCtx_.dt = dt;
        frameCtx_.frameCount = frame;
    }

    Color888 Shade(const SurfaceProperties& surf,
                   const IMaterial& /*mat*/) const override;

    /** Set FrameContext pointer on a ShadeInput (call once per triangle). */
    void PrepareInput(ksl::ShadeInput& input) const;

    /** Get the pre-built per-frame context. */
    const ksl::FrameContext* GetFrameContext() const { return &frameCtx_; }

    /** Get the raw shade function pointer (nullptr if not CPU-ready). */
    ksl::KSLShadeFn GetShadeFnDirect() const {
        return (module_ && module_->HasCPU()) ? module_->GetShadeFn() : nullptr;
    }

    /** Get the shader instance pointer. */
    void* GetShaderInstance() const { return instance_; }

    /** Get the bitmask of required ShadeInput attributes. */
    uint8_t GetRequiredAttribs() const {
        return module_ ? module_->GetRequiredAttribs() : ksl::SHADE_ATTRIB_ALL;
    }

    /** Ensure module is loaded and lights are refreshed (call before extracting fn ptrs). */
    void EnsureReady();

    /** Shade with minimal per-pixel setup (position, normal, uv, viewDir only). */
    Color888 ShadeFast(ksl::ShadeInput& input,
                       float px, float py, float pz,
                       float nx, float ny, float nz,
                       float u, float v,
                       float vdx, float vdy, float vdz) const;

private:
    ksl::KSLModule* module_ = nullptr;
    void* instance_ = nullptr;
    const KSLMaterial* owner_ = nullptr;
    float time_ = 0.0f;
    float dt_ = 0.0f;
    int frame_ = 0;
    ksl::FrameContext frameCtx_;

    KL_BEGIN_FIELDS(KSLShaderBridge)
        KL_FIELD(KSLShaderBridge, dt_, "Dt", 0, 0),
        KL_FIELD(KSLShaderBridge, frame_, "Frame", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(KSLShaderBridge)
        KL_METHOD_AUTO(KSLShaderBridge, SetModule, "Set module"),
        KL_METHOD_AUTO(KSLShaderBridge, SetInstance, "Set instance"),
        KL_METHOD_AUTO(KSLShaderBridge, SetOwner, "Set owner"),
        KL_METHOD_AUTO(KSLShaderBridge, SetTime, "Set time"),
        KL_METHOD_AUTO(KSLShaderBridge, Shade, "Shade")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KSLShaderBridge)
        /* No reflected ctors. */
    KL_END_DESCRIBE(KSLShaderBridge)

};

/**
 * @class KSLMaterial
 * @brief IMaterial wrapper around a KSL shader module.
 *
 * The primary material type for using KSL shaders. Manages a module instance,
 * light array, camera position, and generic shader parameters.
 *
 * Usage from C++:
 *   KSLMaterial mat("spiral");
 *   mat.SetFloat("width", 0.1f);
 *   mesh.SetMaterial(&mat);
 *
 * Usage from KoiloScript:
 *   var mat = KSLMaterial("pbr");
 *   mat.SetFloat("roughness", 0.5);
 *   mat.SetColor("albedo", Color888(200, 180, 160));
 *   mat.AddLight();
 *   mat.SetLightPosition(0, Vector3D(0, 35, 0));
 */
class KSLMaterial : public IMaterial {
public:
    static constexpr int MAX_LIGHTS = 16;

    // --- Static registry access ---

    static void SetRegistry(ksl::KSLRegistry* reg) { s_registry_ = reg; }
    static ksl::KSLRegistry* GetRegistry() { return s_registry_; }
    // --- Constructors ---

    KSLMaterial() : IMaterial(&bridge_) {
        bridge_.SetOwner(this);
    }

    explicit KSLMaterial(const std::string& shaderName) : IMaterial(&bridge_) {
        bridge_.SetOwner(this);
        pendingName_ = shaderName;
        TryResolve();
    }

    ~KSLMaterial() override { Unbind(); }

    KSLMaterial(const KSLMaterial&) = delete;
    KSLMaterial& operator=(const KSLMaterial&) = delete;

    // --- Module binding ---

    /** @brief Attempt to resolve a pending shader name from the registry. */
    bool TryResolve() {
        if (module_) return true;           // already bound
        if (pendingName_.empty()) return false;
        if (!s_registry_) return false;     // registry not available yet
        auto* mod = s_registry_->GetModule(pendingName_);
        if (mod) {
            Bind(mod);
            return true;
        }
        return false;
    }

    /** @brief Ensure module is resolved (called at render time). */
    void EnsureBound() {
        if (!module_ && !pendingName_.empty()) TryResolve();
    }

    bool Bind(ksl::KSLModule* mod) {
        Unbind();
        if (!mod) return false;
        // Allow binding even without CPU (GPU-only is valid for GL rendering)
        module_ = mod;
        if (mod->HasCPU()) {
            instance_ = mod->CreateInstance();
        }
        bridge_.SetModule(mod);
        bridge_.SetInstance(instance_);

        // Replay deferred parameter sets
        for (const auto& dp : deferredParams_) {
            if (instance_)
                module_->SetParam(instance_, dp.name.c_str(), dp.data.data(), dp.type, dp.count);
        }
        deferredParams_.clear();

        for (const auto& das : deferredArraySets_) {
            if (instance_) {
                ksl::ParamList params = module_->GetParams();
                for (int i = 0; i < params.count; ++i) {
                    if (std::string(params.decls[i].name) == das.name &&
                        params.decls[i].type == ksl::ParamType::Vec3 &&
                        das.index >= 0 && das.index < params.decls[i].arraySize) {
                        auto* base = static_cast<char*>(instance_) + params.decls[i].offset;
                        auto* dst = reinterpret_cast<ksl::vec3*>(base) + das.index;
                        *dst = das.value;
                        break;
                    }
                }
            }
        }
        deferredArraySets_.clear();

        return true;
    }

    void Unbind() {
        if (instance_ && module_ && s_registry_) {
            module_->DestroyInstance(instance_);
        }
        instance_ = nullptr;
        module_ = nullptr;
        bridge_.SetModule(nullptr);
        bridge_.SetInstance(nullptr);
    }

    // --- Shader parameters (generic) ---

    void SetParam(const char* name, const void* data, int type, int count = 1) {
        EnsureBound();
        if (module_ && instance_) {
            module_->SetParam(instance_, name, data, type, count);
        } else {
            // Buffer for deferred application when module is bound
            size_t sz = 0;
            switch (static_cast<ksl::ParamType>(type)) {
                case ksl::ParamType::Float: sz = sizeof(float); break;
                case ksl::ParamType::Int:   sz = sizeof(int); break;
                case ksl::ParamType::Bool:  sz = sizeof(bool); break;
                case ksl::ParamType::Vec2:  sz = sizeof(ksl::vec2); break;
                case ksl::ParamType::Vec3:  sz = sizeof(ksl::vec3); break;
                case ksl::ParamType::Vec4:  sz = sizeof(ksl::vec4); break;
            }
            DeferredParam dp;
            dp.name = name;
            dp.data.resize(sz * count);
            std::memcpy(dp.data.data(), data, sz * count);
            dp.type = type;
            dp.count = count;
            deferredParams_.push_back(std::move(dp));
        }
    }

    void SetFloat(const std::string& name, float value) {
        SetParam(name.c_str(), &value, static_cast<int>(ksl::ParamType::Float));
    }

    void SetInt(const std::string& name, int value) {
        SetParam(name.c_str(), &value, static_cast<int>(ksl::ParamType::Int));
    }

    void SetVec3(const std::string& name, float x, float y, float z) {
        ksl::vec3 v{x, y, z};
        SetParam(name.c_str(), &v, static_cast<int>(ksl::ParamType::Vec3));
    }

    void SetVec2(const std::string& name, float x, float y) {
        ksl::vec2 v{x, y};
        SetParam(name.c_str(), &v, static_cast<int>(ksl::ParamType::Vec2));
    }

    void SetColor(const std::string& name, Color888 c) {
        ksl::vec3 v{c.R / 255.0f, c.G / 255.0f, c.B / 255.0f};
        SetParam(name.c_str(), &v, static_cast<int>(ksl::ParamType::Vec3));
    }

    /** @brief Set a vec3 array element (e.g. color palette entry). */
    void SetVec3Array(const std::string& name, int index, float x, float y, float z) {
        EnsureBound();
        if (module_ && instance_) {
            ksl::ParamList params = module_->GetParams();
            for (int i = 0; i < params.count; ++i) {
                if (std::string(params.decls[i].name) == name &&
                    params.decls[i].type == ksl::ParamType::Vec3 &&
                    index >= 0 && index < params.decls[i].arraySize) {
                    auto* base = static_cast<char*>(instance_) + params.decls[i].offset;
                    auto* dst = reinterpret_cast<ksl::vec3*>(base) + index;
                    *dst = {x, y, z};
                    return;
                }
            }
        } else {
            // Buffer for deferred application
            DeferredArraySet das;
            das.name = name;
            das.index = index;
            das.value = {x, y, z};
            deferredArraySets_.push_back(std::move(das));
        }
    }

    /** @brief Set a color palette entry by index. */
    void SetColorAt(const std::string& name, int index, Color888 c) {
        SetVec3Array(name, index, c.R / 255.0f, c.G / 255.0f, c.B / 255.0f);
    }

    /** @brief Initialize a standard rainbow palette (up to 6 colors). */
    void SetRainbowPalette(const std::string& name, int count) {
        const Color888 rainbow[6] = {
            Color888(255,0,0), Color888(255,255,0), Color888(0,255,0),
            Color888(0,255,255), Color888(0,0,255), Color888(255,0,255)
        };
        int n = std::min(count, 6);
        for (int i = 0; i < n; ++i) SetColorAt(name, i, rainbow[i]);
        SetInt("colorCount", count);
    }

    // --- Light management ---

    void AddLight() {
        if (static_cast<int>(lights_.size()) < MAX_LIGHTS) {
            lights_.push_back({});
        }
    }

    int LightCount() const { return static_cast<int>(lights_.size()); }

    void SetLightPosition(int idx, const Vector3D& pos) {
        if (idx >= 0 && idx < static_cast<int>(lights_.size()))
            lights_[idx].position = {static_cast<float>(pos.X),
                                     static_cast<float>(pos.Y),
                                     static_cast<float>(pos.Z)};
    }

    void SetLightPosition(int idx, float x, float y, float z) {
        if (idx >= 0 && idx < static_cast<int>(lights_.size()))
            lights_[idx].position = {x, y, z};
    }

    void SetLightColor(int idx, const Vector3D& color) {
        if (idx >= 0 && idx < static_cast<int>(lights_.size()))
            lights_[idx].color = {static_cast<float>(color.X),
                                  static_cast<float>(color.Y),
                                  static_cast<float>(color.Z)};
    }

    void SetLightColor(int idx, float r, float g, float b) {
        if (idx >= 0 && idx < static_cast<int>(lights_.size()))
            lights_[idx].color = {r, g, b};
    }

    void SetLightIntensity(int idx, float intensity) {
        if (idx >= 0 && idx < static_cast<int>(lights_.size()))
            lights_[idx].intensity = intensity;
    }

    void SetLightFalloff(int idx, float falloff) {
        if (idx >= 0 && idx < static_cast<int>(lights_.size()))
            lights_[idx].falloff = falloff;
    }

    void SetLightCurve(int idx, float curve) {
        if (idx >= 0 && idx < static_cast<int>(lights_.size()))
            lights_[idx].curve = curve;
    }

    const std::vector<ksl::LightData>& GetLights() const { return lights_; }

    // --- Camera ---

    void SetCameraPosition(const Vector3D& p) {
        cameraPos_ = {static_cast<float>(p.X),
                      static_cast<float>(p.Y),
                      static_cast<float>(p.Z)};
    }

    const ksl::vec3& GetCameraPos() const { return cameraPos_; }

    // --- Textures ---

    void SetTexture(int slot, Texture* tex) {
        if (slot >= 0 && slot < 8) {
            if (slot >= static_cast<int>(textures_.size()))
                textures_.resize(slot + 1, nullptr);
            textures_[slot] = tex;
        }
    }

    Texture* GetTexture(int slot) const {
        if (slot >= 0 && slot < static_cast<int>(textures_.size()))
            return textures_[slot];
        return nullptr;
    }

    int TextureCount() const { return static_cast<int>(textures_.size()); }

    // --- State ---

    bool IsBound() const {
        const_cast<KSLMaterial*>(this)->EnsureBound();
        return module_ != nullptr;
    }
    bool IsKSL() const override { return true; }
    ksl::KSLModule* GetModule() const { return module_; }
    void* GetInstance() const { return instance_; }

    void Update() override {
        time_ += dt_;
        frame_++;
        bridge_.SetTime(time_, dt_, frame_);
    }

    void SetDeltaTime(float dt) { dt_ = dt; }
    void SetTime(float t) { time_ = t; bridge_.SetTime(time_, dt_, frame_); }
    float GetTime() const { return time_; }

    // --- Reflection ---

    KL_BEGIN_FIELDS(KSLMaterial)
    KL_END_FIELDS

    KL_BEGIN_METHODS(KSLMaterial)
        KL_METHOD_AUTO(KSLMaterial, IsBound, "Is bound"),
        KL_METHOD_AUTO(KSLMaterial, Unbind, "Unbind"),
        KL_METHOD_AUTO(KSLMaterial, SetFloat, "Set float param"),
        KL_METHOD_AUTO(KSLMaterial, SetInt, "Set int param"),
        KL_METHOD_AUTO(KSLMaterial, SetVec3, "Set vec3 param"),
        KL_METHOD_AUTO(KSLMaterial, SetVec2, "Set vec2 param"),
        KL_METHOD_AUTO(KSLMaterial, SetColor, "Set color param"),
        KL_METHOD_AUTO(KSLMaterial, SetVec3Array, "Set vec3 array element"),
        KL_METHOD_AUTO(KSLMaterial, SetColorAt, "Set color array element"),
        KL_METHOD_AUTO(KSLMaterial, SetRainbowPalette, "Set rainbow palette"),
        KL_METHOD_AUTO(KSLMaterial, AddLight, "Add light"),
        KL_METHOD_AUTO(KSLMaterial, LightCount, "Light count"),
        KL_METHOD_OVLD(KSLMaterial, SetLightPosition, void, int, float, float, float),
        KL_METHOD_OVLD(KSLMaterial, SetLightColor, void, int, float, float, float),
        KL_METHOD_AUTO(KSLMaterial, SetLightIntensity, "Set light intensity"),
        KL_METHOD_AUTO(KSLMaterial, SetLightFalloff, "Set light falloff"),
        KL_METHOD_AUTO(KSLMaterial, SetLightCurve, "Set light curve"),
        KL_METHOD_AUTO(KSLMaterial, SetCameraPosition, "Set camera position"),
        KL_METHOD_AUTO(KSLMaterial, SetTexture, "Set texture"),
        KL_METHOD_AUTO(KSLMaterial, SetTime, "Set time"),
        KL_METHOD_AUTO(KSLMaterial, SetDeltaTime, "Set delta time")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KSLMaterial)
        KL_CTOR(KSLMaterial, const std::string&)
    KL_END_DESCRIBE(KSLMaterial)

private:
    static inline ksl::KSLRegistry* s_registry_ = nullptr;

    KSLShaderBridge bridge_;
    ksl::KSLModule* module_ = nullptr;
    void* instance_ = nullptr;
    std::string pendingName_;

    // Deferred parameter storage (applied when module binds lazily)
    struct DeferredParam {
        std::string name;
        std::vector<uint8_t> data;
        int type;
        int count;

        KL_BEGIN_FIELDS(DeferredParam)
            KL_FIELD(DeferredParam, name, "Name", 0, 0),
            KL_FIELD(DeferredParam, data, "Data", 0, 0),
            KL_FIELD(DeferredParam, type, "Type", -2147483648, 2147483647),
            KL_FIELD(DeferredParam, count, "Count", -2147483648, 2147483647)
        KL_END_FIELDS

        KL_BEGIN_METHODS(DeferredParam)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(DeferredParam)
            /* No reflected ctors. */
        KL_END_DESCRIBE(DeferredParam)

    };
    struct DeferredArraySet {
        std::string name;
        int index;
        ksl::vec3 value;

        KL_BEGIN_FIELDS(DeferredArraySet)
            KL_FIELD(DeferredArraySet, name, "Name", 0, 0),
            KL_FIELD(DeferredArraySet, index, "Index", -2147483648, 2147483647),
            KL_FIELD(DeferredArraySet, value, "Value", 0, 0)
        KL_END_FIELDS

        KL_BEGIN_METHODS(DeferredArraySet)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(DeferredArraySet)
            /* No reflected ctors. */
        KL_END_DESCRIBE(DeferredArraySet)

    };
    std::vector<DeferredParam> deferredParams_;
    std::vector<DeferredArraySet> deferredArraySets_;

    std::vector<ksl::LightData> lights_;
    std::vector<Texture*> textures_;  // Non-owning texture pointers (up to 8 slots)
    ksl::vec3 cameraPos_{};

    float time_ = 0.0f;
    float dt_ = 1.0f / 60.0f;
    int frame_ = 0;
};

// --- KSLShaderBridge implementation (needs KSLMaterial complete type) ---

inline Color888 KSLShaderBridge::Shade(const SurfaceProperties& surf,
                                        const IMaterial& /*mat*/) const {
    // Trigger lazy module resolution if needed
    if (owner_ && !module_) {
        const_cast<KSLMaterial*>(owner_)->EnsureBound();
    }

    if (!module_ || !module_->HasCPU() || !instance_)
        return Color888(255, 0, 200); // pink error

    // Refresh lights
    if (owner_) {
        const auto& lights = owner_->GetLights();
        const_cast<ksl::FrameContext&>(frameCtx_).lights = lights.empty() ? nullptr : lights.data();
        const_cast<ksl::FrameContext&>(frameCtx_).lightCount = static_cast<int>(lights.size());
    }

    ksl::ShadeInput input;
    input.ctx      = &frameCtx_;
    input.position = { surf.position.X, surf.position.Y, surf.position.Z };
    input.normal   = { surf.normal.X,   surf.normal.Y,   surf.normal.Z };
    input.uv       = { surf.uvw.X,      surf.uvw.Y };
    input.viewDir  = { surf.viewDirection.X, surf.viewDirection.Y, surf.viewDirection.Z };

    ksl::vec4 c = module_->Shade(instance_, &input);

    return Color888(
        static_cast<uint8_t>(Mathematics::Constrain(c.x * 255.0f, 0.0f, 255.0f)),
        static_cast<uint8_t>(Mathematics::Constrain(c.y * 255.0f, 0.0f, 255.0f)),
        static_cast<uint8_t>(Mathematics::Constrain(c.z * 255.0f, 0.0f, 255.0f))
    );
}

inline void KSLShaderBridge::PrepareInput(ksl::ShadeInput& input) const {
    if (owner_ && !module_) {
        const_cast<KSLMaterial*>(owner_)->EnsureBound();
    }
    // Refresh lights from owner into frameCtx
    if (owner_) {
        const auto& lights = owner_->GetLights();
        const_cast<ksl::FrameContext&>(frameCtx_).lights = lights.empty() ? nullptr : lights.data();
        const_cast<ksl::FrameContext&>(frameCtx_).lightCount = static_cast<int>(lights.size());

        // Populate textures from owner
        int texCount = owner_->TextureCount();
        auto& ctx = const_cast<ksl::FrameContext&>(frameCtx_);
        ctx.textureCount = 0;
        for (int i = 0; i < texCount && i < 8; ++i) {
            Texture* tex = owner_->GetTexture(i);
            if (tex && tex->GetRGBData()) {
                ctx.textures[i].data = tex->GetRGBData();
                ctx.textures[i].width = static_cast<int>(tex->GetWidth());
                ctx.textures[i].height = static_cast<int>(tex->GetHeight());
                ctx.textures[i].channels = 3;
                ctx.textureCount = i + 1;
            } else {
                ctx.textures[i] = {};
            }
        }
    }
    input.ctx = &frameCtx_;
}

inline void KSLShaderBridge::EnsureReady() {
    if (owner_ && !module_) {
        const_cast<KSLMaterial*>(owner_)->EnsureBound();
    }
    if (owner_) {
        const auto& lights = owner_->GetLights();
        frameCtx_.lights = lights.empty() ? nullptr : lights.data();
        frameCtx_.lightCount = static_cast<int>(lights.size());

        // Populate textures from owner
        int texCount = owner_->TextureCount();
        frameCtx_.textureCount = 0;
        for (int i = 0; i < texCount && i < 8; ++i) {
            Texture* tex = owner_->GetTexture(i);
            if (tex && tex->GetRGBData()) {
                frameCtx_.textures[i].data = tex->GetRGBData();
                frameCtx_.textures[i].width = static_cast<int>(tex->GetWidth());
                frameCtx_.textures[i].height = static_cast<int>(tex->GetHeight());
                frameCtx_.textures[i].channels = 3;
                frameCtx_.textureCount = i + 1;
            } else {
                frameCtx_.textures[i] = {};
            }
        }
    }
}

inline Color888 KSLShaderBridge::ShadeFast(ksl::ShadeInput& input,
                                            float px, float py, float pz,
                                            float nx, float ny, float nz,
                                            float u, float v,
                                            float vdx, float vdy, float vdz) const {
    if (!module_ || !module_->HasCPU() || !instance_)
        return Color888(255, 0, 200);

    input.position = { px, py, pz };
    input.normal   = { nx, ny, nz };
    input.uv       = { u, v };
    input.viewDir  = { vdx, vdy, vdz };

    ksl::vec4 c = module_->Shade(instance_, &input);

    auto to8 = [](float f) -> uint8_t {
        return f <= 0.0f ? 0 : f >= 1.0f ? 255 : static_cast<uint8_t>(f * 255.0f);
    };
    return Color888(to8(c.x), to8(c.y), to8(c.z));
}

} // namespace koilo
