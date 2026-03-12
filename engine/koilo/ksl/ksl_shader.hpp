// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ksl_types.hpp"
#include <cstddef>
#include <cstdint>


namespace ksl {

// --- Texture Sampling (opaque on CPU, maps to sampler2D on GPU) ---

struct TextureSampler {
    const uint8_t* data = nullptr;
    int width = 0;
    int height = 0;
    int channels = 3;
};

inline vec4 sample(const TextureSampler& tex, const vec2& uv) {
    if (!tex.data || tex.width <= 0 || tex.height <= 0) return vec4(1.0f, 0.0f, 0.78f, 1.0f);
    int px = static_cast<int>(uv.x * tex.width) % tex.width;
    int py = static_cast<int>(uv.y * tex.height) % tex.height;
    if (px < 0) px += tex.width;
    if (py < 0) py += tex.height;
    int idx = (py * tex.width + px) * tex.channels;
    float r = tex.data[idx] / 255.0f;
    float g = tex.data[idx + 1] / 255.0f;
    float b = tex.data[idx + 2] / 255.0f;
    float a = tex.channels >= 4 ? tex.data[idx + 3] / 255.0f : 1.0f;
    return {r, g, b, a};
}

// --- Light Data ---

struct LightData {
    vec3 position;
    vec3 color;
    float intensity = 1.0f;
    float falloff = 100.0f;
    float curve = 2.0f;
};

// --- Per-Frame Context (set once per frame, shared across all pixels) ---

struct FrameContext {
    float time = 0.0f;
    float dt = 0.0f;
    int frameCount = 0;

    const LightData* lights = nullptr;
    int lightCount = 0;

    TextureSampler textures[8];
    int textureCount = 0;

    const float* audioSamples = nullptr;
    int sampleCount = 0;
    const float* audioSpectrum = nullptr;
    int spectrumCount = 0;
};

// --- Per-Pixel Shade Input ---

struct ShadeInput {
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec3 viewDir;
    const FrameContext* ctx = nullptr;
};

// --- Shader Base Class ---

struct Shader {
    virtual ~Shader() = default;
    virtual vec4 shade(const ShadeInput& in) const = 0;
};

// --- Parameter Introspection ---

enum class ParamType : uint8_t {
    Float, Int, Bool, Vec2, Vec3, Vec4
};

enum class ParamFlags : uint8_t {
    None = 0,
    Array = 1
};

struct ParamDecl {
    const char* name;
    ParamType type;
    const char* desc;
    size_t offset;
    float minVal;
    float maxVal;
    ParamFlags flags = ParamFlags::None;
    int arraySize = 0;
};

struct ParamList {
    const ParamDecl* decls;
    int count;
};

// Type-to-enum mapping
template<typename T> inline ParamType TypeID();
template<> inline ParamType TypeID<float>()  { return ParamType::Float; }
template<> inline ParamType TypeID<int>()    { return ParamType::Int; }
template<> inline ParamType TypeID<bool>()   { return ParamType::Bool; }
template<> inline ParamType TypeID<vec2>()   { return ParamType::Vec2; }
template<> inline ParamType TypeID<vec3>()   { return ParamType::Vec3; }
template<> inline ParamType TypeID<vec4>()   { return ParamType::Vec4; }

// --- Shade attribute requirement flags ---

enum ShadeAttrib : uint8_t {
    SHADE_ATTRIB_NONE    = 0,
    SHADE_ATTRIB_POS     = 1 << 0,
    SHADE_ATTRIB_UV      = 1 << 1,
    SHADE_ATTRIB_VIEWDIR = 1 << 2,
    SHADE_ATTRIB_NORMAL  = 1 << 3,
    SHADE_ATTRIB_ALL     = 0xFF
};

// --- C ABI for .so modules ---

struct KSLShaderInfo {
    char name[64];
    uint32_t version;
    uint32_t paramCount;
    uint8_t requiredAttribs;  ///< Bitmask of ShadeAttrib flags the shader reads
};

// Function pointer types for dlopen'd modules
using KSLCreateFn     = void* (*)();
using KSLDestroyFn    = void (*)(void*);
using KSLSetParamFn   = void (*)(void*, const char*, const void*, int, int);
using KSLShadeFn      = vec4 (*)(void*, const ShadeInput*);
using KSLInfoFn       = const KSLShaderInfo* (*)();
using KSLParamsFn     = const ParamDecl* (*)(int*);

} // namespace ksl

