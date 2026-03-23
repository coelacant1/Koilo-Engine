// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file rhi_types.hpp
 * @brief Render Hardware Interface - opaque handles, format enums, and
 *        descriptor structs for backend-agnostic GPU resource management.
 *
 * All types are pure data - no API calls, no allocations.  The IRHIDevice
 * interface consumes these to create/destroy actual GPU resources.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */
#pragma once
#include <cstdint>
#include <cstddef>
#include <array>

namespace koilo::rhi {

// -- Opaque handles --------------------------------------------------
// Each handle is a strongly-typed uint32_t index.  The null sentinel
// (0) means "no resource".  Backends map indices to native objects
// via internal slot arrays.

struct RHIBuffer          { uint32_t id = 0; bool IsValid() const { return id != 0; } };
struct RHITexture         { uint32_t id = 0; bool IsValid() const { return id != 0; } };
struct RHIShader          { uint32_t id = 0; bool IsValid() const { return id != 0; } };
struct RHIPipeline        { uint32_t id = 0; bool IsValid() const { return id != 0; } };
struct RHIRenderPass      { uint32_t id = 0; bool IsValid() const { return id != 0; } };
struct RHIDescriptorSet   { uint32_t id = 0; bool IsValid() const { return id != 0; } };
struct RHIFramebuffer     { uint32_t id = 0; bool IsValid() const { return id != 0; } };

inline bool operator==(RHIBuffer a, RHIBuffer b)             { return a.id == b.id; }
inline bool operator==(RHITexture a, RHITexture b)           { return a.id == b.id; }
inline bool operator==(RHIShader a, RHIShader b)             { return a.id == b.id; }
inline bool operator==(RHIPipeline a, RHIPipeline b)         { return a.id == b.id; }
inline bool operator==(RHIRenderPass a, RHIRenderPass b)     { return a.id == b.id; }
inline bool operator==(RHIDescriptorSet a, RHIDescriptorSet b) { return a.id == b.id; }
inline bool operator==(RHIFramebuffer a, RHIFramebuffer b)   { return a.id == b.id; }

inline bool operator!=(RHIBuffer a, RHIBuffer b)             { return a.id != b.id; }
inline bool operator!=(RHITexture a, RHITexture b)           { return a.id != b.id; }
inline bool operator!=(RHIShader a, RHIShader b)             { return a.id != b.id; }
inline bool operator!=(RHIPipeline a, RHIPipeline b)         { return a.id != b.id; }
inline bool operator!=(RHIRenderPass a, RHIRenderPass b)     { return a.id != b.id; }
inline bool operator!=(RHIDescriptorSet a, RHIDescriptorSet b) { return a.id != b.id; }
inline bool operator!=(RHIFramebuffer a, RHIFramebuffer b)   { return a.id != b.id; }

// -- Pixel / depth formats -------------------------------------------
enum class RHIFormat : uint8_t {
    Undefined = 0,
    // Color
    R8_Unorm,
    RG8_Unorm,
    RGB8_Unorm,
    RGBA8_Unorm,
    BGRA8_Unorm,
    RGBA8_SRGB,
    BGRA8_SRGB,
    // HDR / float
    RG16F,
    RGBA16F,
    R32F,
    RG32F,
    RGB32F,
    RGBA32F,
    // Depth / stencil
    D16_Unorm,
    D24_Unorm_S8_Uint,
    D32F,
    D32F_S8_Uint,
};

/// Bytes per pixel for a given format (0 if undefined/compressed).
inline uint8_t RHIFormatBytesPerPixel(RHIFormat fmt) {
    switch (fmt) {
        case RHIFormat::R8_Unorm:           return 1;
        case RHIFormat::RG8_Unorm:          return 2;
        case RHIFormat::RGB8_Unorm:         return 3;
        case RHIFormat::RGBA8_Unorm:
        case RHIFormat::BGRA8_Unorm:
        case RHIFormat::RGBA8_SRGB:
        case RHIFormat::BGRA8_SRGB:        return 4;
        case RHIFormat::RG16F:              return 4;
        case RHIFormat::RGBA16F:            return 8;
        case RHIFormat::R32F:               return 4;
        case RHIFormat::RG32F:              return 8;
        case RHIFormat::RGB32F:             return 12;
        case RHIFormat::RGBA32F:            return 16;
        case RHIFormat::D16_Unorm:          return 2;
        case RHIFormat::D24_Unorm_S8_Uint:  return 4;
        case RHIFormat::D32F:               return 4;
        case RHIFormat::D32F_S8_Uint:       return 5;
        default:                            return 0;
    }
}

inline bool RHIFormatIsDepth(RHIFormat fmt) {
    return fmt == RHIFormat::D16_Unorm
        || fmt == RHIFormat::D24_Unorm_S8_Uint
        || fmt == RHIFormat::D32F
        || fmt == RHIFormat::D32F_S8_Uint;
}

inline bool RHIFormatHasStencil(RHIFormat fmt) {
    return fmt == RHIFormat::D24_Unorm_S8_Uint
        || fmt == RHIFormat::D32F_S8_Uint;
}

// -- Buffer usage ----------------------------------------------------
enum class RHIBufferUsage : uint8_t {
    Vertex   = 1 << 0,
    Index    = 1 << 1,
    Uniform  = 1 << 2,
    Storage  = 1 << 3,
    Staging  = 1 << 4,
    Indirect = 1 << 5,
};

inline RHIBufferUsage operator|(RHIBufferUsage a, RHIBufferUsage b) {
    return static_cast<RHIBufferUsage>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline RHIBufferUsage operator&(RHIBufferUsage a, RHIBufferUsage b) {
    return static_cast<RHIBufferUsage>(
        static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
inline bool HasFlag(RHIBufferUsage val, RHIBufferUsage flag) {
    return (static_cast<uint8_t>(val) & static_cast<uint8_t>(flag)) != 0;
}

// -- Texture usage ---------------------------------------------------
enum class RHITextureUsage : uint8_t {
    Sampled       = 1 << 0,
    RenderTarget  = 1 << 1,
    DepthStencil  = 1 << 2,
    TransferSrc   = 1 << 3,
    TransferDst   = 1 << 4,
    Storage       = 1 << 5,
};

inline RHITextureUsage operator|(RHITextureUsage a, RHITextureUsage b) {
    return static_cast<RHITextureUsage>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline RHITextureUsage operator&(RHITextureUsage a, RHITextureUsage b) {
    return static_cast<RHITextureUsage>(
        static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
inline bool HasFlag(RHITextureUsage val, RHITextureUsage flag) {
    return (static_cast<uint8_t>(val) & static_cast<uint8_t>(flag)) != 0;
}

// -- Sampler filter ---------------------------------------------------
enum class RHISamplerFilter : uint8_t {
    Nearest,
    Linear,
};

// -- Shader stages ---------------------------------------------------
enum class RHIShaderStage : uint8_t {
    Vertex   = 1 << 0,
    Fragment = 1 << 1,
    Compute  = 1 << 2,
};

inline RHIShaderStage operator|(RHIShaderStage a, RHIShaderStage b) {
    return static_cast<RHIShaderStage>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

// -- Primitive topology ----------------------------------------------
enum class RHITopology : uint8_t {
    TriangleList  = 0,
    TriangleStrip = 1,
    LineList      = 2,
    LineStrip     = 3,
    PointList     = 4,
};

// -- Comparison / blend / cull ---------------------------------------
enum class RHICompareOp : uint8_t {
    Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always,
};

enum class RHICullMode : uint8_t { None, Front, Back, FrontAndBack };
enum class RHIFrontFace : uint8_t { CounterClockwise, Clockwise };
enum class RHIPolygonMode : uint8_t { Fill, Line, Point };

enum class RHIBlendFactor : uint8_t {
    Zero, One, SrcAlpha, OneMinusSrcAlpha, DstAlpha, OneMinusDstAlpha,
    SrcColor, OneMinusSrcColor, DstColor, OneMinusDstColor,
};

enum class RHIBlendOp : uint8_t { Add, Subtract, ReverseSubtract, Min, Max };

enum class RHILoadOp : uint8_t { Load, Clear, DontCare };
enum class RHIStoreOp : uint8_t { Store, DontCare };

// -- Vertex attribute ------------------------------------------------
struct RHIVertexAttr {
    uint32_t  location = 0;
    RHIFormat format   = RHIFormat::Undefined;
    uint32_t  offset   = 0;
};

// -- Descriptor structs ----------------------------------------------

/// Buffer creation descriptor.
struct RHIBufferDesc {
    uint64_t        size    = 0;
    RHIBufferUsage  usage   = RHIBufferUsage::Vertex;
    bool            hostVisible = false;  ///< CPU-mappable?
    const char*     debugName   = nullptr;
};

/// Texture creation descriptor.
struct RHITextureDesc {
    uint32_t        width  = 1;
    uint32_t        height = 1;
    uint32_t        depth  = 1;
    uint32_t        mipLevels   = 1;
    uint32_t        arrayLayers = 1;
    RHIFormat       format = RHIFormat::RGBA8_Unorm;
    RHITextureUsage usage  = RHITextureUsage::Sampled;
    RHISamplerFilter filter = RHISamplerFilter::Linear;
    const char*     debugName = nullptr;
};

/// Blend state for a single color attachment.
struct RHIBlendState {
    bool           enabled      = false;
    RHIBlendFactor srcColor     = RHIBlendFactor::SrcAlpha;
    RHIBlendFactor dstColor     = RHIBlendFactor::OneMinusSrcAlpha;
    RHIBlendOp     colorOp      = RHIBlendOp::Add;
    RHIBlendFactor srcAlpha     = RHIBlendFactor::One;
    RHIBlendFactor dstAlpha     = RHIBlendFactor::OneMinusSrcAlpha;
    RHIBlendOp     alphaOp      = RHIBlendOp::Add;
};

/// Rasterizer state embedded in the pipeline descriptor.
struct RHIRasterizerState {
    RHICullMode    cullMode    = RHICullMode::Back;
    RHIFrontFace   frontFace   = RHIFrontFace::CounterClockwise;
    RHIPolygonMode polygonMode = RHIPolygonMode::Fill;
    bool           depthClamp  = false;
};

/// Depth/stencil state embedded in the pipeline descriptor.
struct RHIDepthStencilState {
    bool         depthTest   = true;
    bool         depthWrite  = true;
    RHICompareOp depthOp     = RHICompareOp::Less;
    bool         stencilTest = false;
};

/// Render flags derived from shader metadata (depthTest, cullMode, etc.).
/// Used by the material binder to create pipeline variants automatically.
struct RenderFlags {
    RHIDepthStencilState depthStencil = {};
    RHICullMode          cullMode     = RHICullMode::Back;
};

/// Graphics pipeline creation descriptor.
struct RHIPipelineDesc {
    RHIShader       vertexShader   = {};
    RHIShader       fragmentShader = {};
    RHIRenderPass   renderPass     = {};

    // Vertex input
    static constexpr uint32_t kMaxVertexAttrs = 8;
    RHIVertexAttr   vertexAttrs[kMaxVertexAttrs] = {};
    uint32_t        vertexAttrCount = 0;
    uint32_t        vertexStride    = 0;

    // Fixed-function state
    RHITopology          topology     = RHITopology::TriangleList;
    RHIRasterizerState   rasterizer   = {};
    RHIDepthStencilState depthStencil = {};
    RHIBlendState        blend        = {};

    // Pipeline layout hint: determines which descriptor set layout to use
    // Scene = 3-set layout (scene+material+textures), Blit = sampler+optional UBO
    enum class LayoutHint : uint8_t {
        Scene = 0,  // Set 0: scene UBO+SSBO, Set 1: material UBO, Set 2: textures
        Blit  = 1,  // Set 0: sampler, Set 1: optional UBO
    };
    LayoutHint layoutHint = LayoutHint::Scene;

    const char* debugName = nullptr;
};

/// Color attachment descriptor for render pass creation.
struct RHIColorAttachment {
    RHIFormat  format  = RHIFormat::RGBA8_Unorm;
    RHILoadOp  loadOp  = RHILoadOp::Clear;
    RHIStoreOp storeOp = RHIStoreOp::Store;
    bool sampleAfterPass = false; // if true, final layout = SHADER_READ_ONLY_OPTIMAL
};

/// Render pass creation descriptor.
struct RHIRenderPassDesc {
    static constexpr uint32_t kMaxColorAttachments = 4;
    RHIColorAttachment colorAttachments[kMaxColorAttachments] = {};
    uint32_t           colorAttachmentCount = 0;

    bool       hasDepth       = true;
    RHIFormat  depthFormat    = RHIFormat::D24_Unorm_S8_Uint;
    RHILoadOp  depthLoadOp   = RHILoadOp::Clear;
    RHIStoreOp depthStoreOp  = RHIStoreOp::DontCare;
};

/// Viewport rectangle.
struct RHIViewport {
    float x = 0, y = 0;
    float width = 0, height = 0;
    float minDepth = 0.0f, maxDepth = 1.0f;
};

/// Scissor rectangle.
struct RHIScissor {
    int32_t  x = 0, y = 0;
    uint32_t width = 0, height = 0;
};

/// Clear values for a render pass begin.
struct RHIClearValue {
    float color[4]  = {0.0f, 0.0f, 0.0f, 1.0f};
    float depth      = 1.0f;
    uint8_t stencil  = 0;
};

} // namespace koilo::rhi
