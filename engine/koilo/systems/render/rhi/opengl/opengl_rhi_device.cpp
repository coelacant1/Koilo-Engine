// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file opengl_rhi_device.cpp
 * @brief OpenGL 3.3+ implementation of IRHIDevice - resource management,
 *        immediate-mode command recording, and presentation.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */
#include "opengl_rhi_device.hpp"
#include <koilo/systems/display/backends/gpu/openglbackend.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace koilo::rhi {

// -- Format conversion tables ----------------------------------------

GLenum OpenGLRHIDevice::ToGLInternalFormat(RHIFormat fmt) {
    switch (fmt) {
        case RHIFormat::R8_Unorm:           return GL_R8;
        case RHIFormat::RG8_Unorm:          return GL_RG8;
        case RHIFormat::RGB8_Unorm:         return GL_RGB8;
        case RHIFormat::RGBA8_Unorm:        return GL_RGBA8;
        case RHIFormat::BGRA8_Unorm:        return GL_RGBA8;
        case RHIFormat::RGBA8_SRGB:         return GL_SRGB8_ALPHA8;
        case RHIFormat::BGRA8_SRGB:         return GL_SRGB8_ALPHA8;
        case RHIFormat::RG16F:              return GL_RG16F;
        case RHIFormat::RGBA16F:            return GL_RGBA16F;
        case RHIFormat::RGBA32F:            return GL_RGBA32F;
        case RHIFormat::RGB32F:             return GL_RGB32F;
        case RHIFormat::RG32F:              return GL_RG32F;
        case RHIFormat::R32F:               return GL_R32F;
        case RHIFormat::D16_Unorm:          return GL_DEPTH_COMPONENT16;
        case RHIFormat::D24_Unorm_S8_Uint:  return GL_DEPTH24_STENCIL8;
        case RHIFormat::D32F:               return GL_DEPTH_COMPONENT32F;
        case RHIFormat::D32F_S8_Uint:       return GL_DEPTH32F_STENCIL8;
        default:                            return 0;
    }
}

GLenum OpenGLRHIDevice::ToGLPixelFormat(RHIFormat fmt) {
    switch (fmt) {
        case RHIFormat::R8_Unorm:
        case RHIFormat::R32F:               return GL_RED;
        case RHIFormat::RG8_Unorm:
        case RHIFormat::RG16F:
        case RHIFormat::RG32F:              return GL_RG;
        case RHIFormat::RGB8_Unorm:
        case RHIFormat::RGB32F:             return GL_RGB;
        case RHIFormat::RGBA8_Unorm:
        case RHIFormat::RGBA8_SRGB:
        case RHIFormat::RGBA16F:
        case RHIFormat::RGBA32F:            return GL_RGBA;
        case RHIFormat::BGRA8_Unorm:
        case RHIFormat::BGRA8_SRGB:        return GL_BGRA;
        case RHIFormat::D16_Unorm:
        case RHIFormat::D32F:               return GL_DEPTH_COMPONENT;
        case RHIFormat::D24_Unorm_S8_Uint:
        case RHIFormat::D32F_S8_Uint:       return GL_DEPTH_STENCIL;
        default:                            return 0;
    }
}

GLenum OpenGLRHIDevice::ToGLPixelType(RHIFormat fmt) {
    switch (fmt) {
        case RHIFormat::R8_Unorm:
        case RHIFormat::RG8_Unorm:
        case RHIFormat::RGB8_Unorm:
        case RHIFormat::RGBA8_Unorm:
        case RHIFormat::BGRA8_Unorm:
        case RHIFormat::RGBA8_SRGB:
        case RHIFormat::BGRA8_SRGB:        return GL_UNSIGNED_BYTE;
        case RHIFormat::RG16F:
        case RHIFormat::RGBA16F:            return GL_HALF_FLOAT;
        case RHIFormat::RGBA32F:
        case RHIFormat::R32F:
        case RHIFormat::D32F:               return GL_FLOAT;
        case RHIFormat::D16_Unorm:          return GL_UNSIGNED_SHORT;
        case RHIFormat::D24_Unorm_S8_Uint:  return GL_UNSIGNED_INT_24_8;
        case RHIFormat::D32F_S8_Uint:       return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
        default:                            return GL_UNSIGNED_BYTE;
    }
}

GLenum OpenGLRHIDevice::ToGLTopology(RHITopology t) {
    switch (t) {
        case RHITopology::TriangleList:  return GL_TRIANGLES;
        case RHITopology::TriangleStrip: return GL_TRIANGLE_STRIP;
        case RHITopology::LineList:      return GL_LINES;
        case RHITopology::LineStrip:     return GL_LINE_STRIP;
        case RHITopology::PointList:     return GL_POINTS;
        default:                         return GL_TRIANGLES;
    }
}

GLenum OpenGLRHIDevice::ToGLCompareFunc(RHICompareOp op) {
    switch (op) {
        case RHICompareOp::Never:        return GL_NEVER;
        case RHICompareOp::Less:         return GL_LESS;
        case RHICompareOp::Equal:        return GL_EQUAL;
        case RHICompareOp::LessEqual:    return GL_LEQUAL;
        case RHICompareOp::Greater:      return GL_GREATER;
        case RHICompareOp::NotEqual:     return GL_NOTEQUAL;
        case RHICompareOp::GreaterEqual: return GL_GEQUAL;
        case RHICompareOp::Always:       return GL_ALWAYS;
        default:                         return GL_LESS;
    }
}

GLenum OpenGLRHIDevice::ToGLBlendFactor(RHIBlendFactor f) {
    switch (f) {
        case RHIBlendFactor::Zero:              return GL_ZERO;
        case RHIBlendFactor::One:               return GL_ONE;
        case RHIBlendFactor::SrcAlpha:          return GL_SRC_ALPHA;
        case RHIBlendFactor::OneMinusSrcAlpha:  return GL_ONE_MINUS_SRC_ALPHA;
        case RHIBlendFactor::DstAlpha:          return GL_DST_ALPHA;
        case RHIBlendFactor::OneMinusDstAlpha:  return GL_ONE_MINUS_DST_ALPHA;
        case RHIBlendFactor::SrcColor:          return GL_SRC_COLOR;
        case RHIBlendFactor::OneMinusSrcColor:  return GL_ONE_MINUS_SRC_COLOR;
        case RHIBlendFactor::DstColor:          return GL_DST_COLOR;
        case RHIBlendFactor::OneMinusDstColor:  return GL_ONE_MINUS_DST_COLOR;
        default:                                return GL_ONE;
    }
}

GLenum OpenGLRHIDevice::ToGLBlendOp(RHIBlendOp op) {
    switch (op) {
        case RHIBlendOp::Add:             return GL_FUNC_ADD;
        case RHIBlendOp::Subtract:        return GL_FUNC_SUBTRACT;
        case RHIBlendOp::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
        case RHIBlendOp::Min:             return GL_MIN;
        case RHIBlendOp::Max:             return GL_MAX;
        default:                          return GL_FUNC_ADD;
    }
}

GLint OpenGLRHIDevice::VertexAttrComponentCount(RHIFormat fmt) {
    switch (fmt) {
        case RHIFormat::R8_Unorm:
        case RHIFormat::R32F:     return 1;
        case RHIFormat::RG8_Unorm:
        case RHIFormat::RG16F:
        case RHIFormat::RG32F:    return 2;
        case RHIFormat::RGB8_Unorm:
        case RHIFormat::RGB32F:   return 3;
        case RHIFormat::RGBA8_Unorm:
        case RHIFormat::BGRA8_Unorm:
        case RHIFormat::RGBA16F:
        case RHIFormat::RGBA32F:  return 4;
        default:                  return 4;
    }
}

GLenum OpenGLRHIDevice::VertexAttrGLType(RHIFormat fmt) {
    switch (fmt) {
        case RHIFormat::R8_Unorm:
        case RHIFormat::RG8_Unorm:
        case RHIFormat::RGB8_Unorm:
        case RHIFormat::RGBA8_Unorm:
        case RHIFormat::BGRA8_Unorm:  return GL_UNSIGNED_BYTE;
        case RHIFormat::RG16F:
        case RHIFormat::RGBA16F:      return GL_HALF_FLOAT;
        case RHIFormat::R32F:
        case RHIFormat::RG32F:
        case RHIFormat::RGB32F:
        case RHIFormat::RGBA32F:      return GL_FLOAT;
        default:                      return GL_FLOAT;
    }
}

// -- Constructor / Destructor ----------------------------------------

OpenGLRHIDevice::OpenGLRHIDevice(OpenGLBackend* display)
    : display_(display)
{
    buffers_.slots.emplace_back();
    textures_.slots.emplace_back();
    shaders_.slots.emplace_back();
    pipelines_.slots.emplace_back();
    renderPasses_.slots.emplace_back();
    framebuffers_.slots.emplace_back();
}

OpenGLRHIDevice::~OpenGLRHIDevice() {
    if (initialized_) Shutdown();
}

// -- Lifecycle -------------------------------------------------------

bool OpenGLRHIDevice::Initialize() {
    if (initialized_) return true;
    if (!display_ || !display_->IsInitialized()) {
        KL_ERR("RHI", "OpenGLBackend display not initialized");
        return false;
    }

    // Create persistent VAO
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    // Create push-constant emulation UBO
    glGenBuffers(1, &pushConstantUBO_);
    glBindBuffer(GL_UNIFORM_BUFFER, pushConstantUBO_);
    glBufferData(GL_UNIFORM_BUFFER, kPushConstantMaxSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Query device limits
    GLint val = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &val);
    limits_.maxTextureSize = static_cast<uint32_t>(val);
    limits_.maxCubeMapSize = static_cast<uint32_t>(val);

    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &val);
    limits_.maxTexture3DSize = static_cast<uint32_t>(val);

    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &val);
    limits_.maxFramebufferWidth  = static_cast<uint32_t>(val);
    limits_.maxFramebufferHeight = static_cast<uint32_t>(val);

    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &val);
    limits_.maxColorAttachments = static_cast<uint32_t>(val);

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &val);
    limits_.maxVertexAttributes = static_cast<uint32_t>(val);
    limits_.maxVertexBindings   = limits_.maxVertexAttributes;

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &val);
    limits_.maxUniformBufferSize = static_cast<uint32_t>(val);

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &val);
    limits_.maxSamplers = static_cast<uint32_t>(val);

    GLfloat fval = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fval);
    limits_.maxAnisotropy = fval;

    limits_.maxPushConstantSize    = kPushConstantMaxSize;
    limits_.maxBoundDescriptorSets = 4;
    limits_.maxStorageBufferSize   = 0;
    limits_.maxDrawIndirectCount   = 1;

    // Query supported features
    uint16_t featureBits = 0;
    featureBits |= static_cast<uint16_t>(RHIFeature::StorageBuffers);
    featureBits |= static_cast<uint16_t>(RHIFeature::PushConstants);
    featureBits |= static_cast<uint16_t>(RHIFeature::DepthClamp);

    // GL_ARB_timer_query is core since GL 3.3
    tsSupported_ = true;
    featureBits |= static_cast<uint16_t>(RHIFeature::TimestampQueries);
    limits_.timestampPeriod = 1.0; // GL timestamps are in nanoseconds

    supportedFeatures_ = static_cast<RHIFeature>(featureBits);

    initialized_ = true;
    KL_LOG("RHI", "OpenGL RHI initialized");
    return true;
}

void OpenGLRHIDevice::Shutdown() {
    if (!initialized_) return;
    glFinish();

    for (uint32_t i = 1; i < static_cast<uint32_t>(framebuffers_.slots.size()); ++i) {
        auto& s = framebuffers_.slots[i];
        if (s.fbo) glDeleteFramebuffers(1, &s.fbo);
    }
    for (uint32_t i = 1; i < static_cast<uint32_t>(pipelines_.slots.size()); ++i) {
        auto& s = pipelines_.slots[i];
        if (s.program) glDeleteProgram(s.program);
    }
    for (uint32_t i = 1; i < static_cast<uint32_t>(shaders_.slots.size()); ++i) {
        auto& s = shaders_.slots[i];
        if (s.shader) glDeleteShader(s.shader);
    }
    for (uint32_t i = 1; i < static_cast<uint32_t>(textures_.slots.size()); ++i) {
        auto& s = textures_.slots[i];
        if (s.sampler)  glDeleteSamplers(1, &s.sampler);
        if (s.texture)  glDeleteTextures(1, &s.texture);
    }
    for (uint32_t i = 1; i < static_cast<uint32_t>(buffers_.slots.size()); ++i) {
        auto& s = buffers_.slots[i];
        if (s.buffer) glDeleteBuffers(1, &s.buffer);
    }

    if (pushConstantUBO_) { glDeleteBuffers(1, &pushConstantUBO_); pushConstantUBO_ = 0; }
    if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    lastAttrMask_ = 0;

    // Clean up timestamp query objects
    if (!tsQueries_.empty())
        glDeleteQueries(static_cast<GLsizei>(tsQueries_.size()), tsQueries_.data());
    if (!tsQueriesPrev_.empty())
        glDeleteQueries(static_cast<GLsizei>(tsQueriesPrev_.size()), tsQueriesPrev_.data());
    tsQueries_.clear();
    tsQueriesPrev_.clear();

    initialized_ = false;
    KL_LOG("RHI", "OpenGL RHI shut down");
}

// -- Capabilities ----------------------------------------------------

bool OpenGLRHIDevice::SupportsFeature(RHIFeature feature) const {
    return HasFeature(supportedFeatures_, feature);
}

RHILimits OpenGLRHIDevice::GetLimits() const {
    return limits_;
}

// -- Buffer ----------------------------------------------------------

RHIBuffer OpenGLRHIDevice::CreateBuffer(const RHIBufferDesc& desc) {
    BufferSlot slot{};
    glGenBuffers(1, &slot.buffer);

    if (HasFlag(desc.usage, RHIBufferUsage::Uniform))
        slot.target = GL_UNIFORM_BUFFER;
    else if (HasFlag(desc.usage, RHIBufferUsage::Index))
        slot.target = GL_ELEMENT_ARRAY_BUFFER;
    else
        slot.target = GL_ARRAY_BUFFER;

    slot.size        = static_cast<GLsizeiptr>(desc.size);
    slot.hostVisible = desc.hostVisible;

    GLenum usage = desc.hostVisible ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    glBindBuffer(slot.target, slot.buffer);
    glBufferData(slot.target, slot.size, nullptr, usage);
    glBindBuffer(slot.target, 0);

    return {buffers_.Alloc(slot)};
}

void OpenGLRHIDevice::DestroyBuffer(RHIBuffer handle) {
    if (!buffers_.Valid(handle.id)) return;
    auto& s = buffers_.Get(handle.id);
    if (s.buffer) glDeleteBuffers(1, &s.buffer);
    if (boundVertexBuf_ == handle.id) { boundVertexBuf_ = 0; boundVertexBufPtr_ = nullptr; vertexStateDirty_ = true; }
    if (boundIndexBuf_  == handle.id) { boundIndexBuf_  = 0; boundIndexBufPtr_  = nullptr; }
    buffers_.Free(handle.id);
}

// -- Texture ---------------------------------------------------------

RHITexture OpenGLRHIDevice::CreateTexture(const RHITextureDesc& desc) {
    TextureSlot slot{};
    slot.width          = desc.width;
    slot.height         = desc.height;
    slot.internalFormat = ToGLInternalFormat(desc.format);
    slot.pixelFormat    = ToGLPixelFormat(desc.format);
    slot.pixelType      = ToGLPixelType(desc.format);

    // For single-channel formats, set alignment to 1
    GLint prevAlignment = 4;
    if (slot.pixelFormat == GL_RED) {
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    glGenTextures(1, &slot.texture);
    glBindTexture(GL_TEXTURE_2D, slot.texture);

    glTexImage2D(GL_TEXTURE_2D, 0, slot.internalFormat,
                 desc.width, desc.height, 0,
                 slot.pixelFormat, slot.pixelType, nullptr);

    // Set filtering on both the texture object AND a separate sampler.
    // The sampler overrides texture params when bound, but setting them
    // on the texture too avoids blurry sampling if the sampler is ever
    // unbound (e.g., GL_TEXTURE_MIN_FILTER defaults to MIPMAP_LINEAR).
    GLint glFilter = (desc.filter == RHISamplerFilter::Nearest) ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // For single-channel (R8) textures, apply swizzle so the red channel
    // is read as alpha (matching the legacy UIGLRenderer behavior).
    // This makes the texture appear as (1,1,1,R) when sampled.
    if (slot.pixelFormat == GL_RED) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
    }

    glGenSamplers(1, &slot.sampler);
    glSamplerParameteri(slot.sampler, GL_TEXTURE_MIN_FILTER, glFilter);
    glSamplerParameteri(slot.sampler, GL_TEXTURE_MAG_FILTER, glFilter);
    glSamplerParameteri(slot.sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(slot.sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    if (slot.pixelFormat == GL_RED)
        glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlignment);

    return {textures_.Alloc(slot)};
}

void OpenGLRHIDevice::DestroyTexture(RHITexture handle) {
    if (!textures_.Valid(handle.id)) return;
    auto& s = textures_.Get(handle.id);
    if (s.sampler)  glDeleteSamplers(1, &s.sampler);
    if (s.texture)  glDeleteTextures(1, &s.texture);
    textures_.Free(handle.id);
}

// -- Shader ----------------------------------------------------------

RHIShader OpenGLRHIDevice::CreateShader(RHIShaderStage stage,
                                         const void* code, size_t codeSize) {
    GLenum glStage = (static_cast<uint8_t>(stage) & static_cast<uint8_t>(RHIShaderStage::Vertex))
                     ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;

    GLuint shader = glCreateShader(glStage);
    const auto* src = static_cast<const char*>(code);
    GLint len = static_cast<GLint>(codeSize);
    glShaderSource(shader, 1, &src, &len);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char info[512];
        glGetShaderInfoLog(shader, sizeof(info), nullptr, info);
        KL_ERR("RHI", "Shader compile error: %s", info);
        glDeleteShader(shader);
        return {0};
    }

    ShaderSlot slot{};
    slot.shader = shader;
    slot.stage  = stage;
    return {shaders_.Alloc(slot)};
}

void OpenGLRHIDevice::DestroyShader(RHIShader handle) {
    if (!shaders_.Valid(handle.id)) return;
    auto& s = shaders_.Get(handle.id);
    if (s.shader) glDeleteShader(s.shader);
    shaders_.Free(handle.id);
}

// -- Pipeline --------------------------------------------------------

RHIPipeline OpenGLRHIDevice::CreatePipeline(const RHIPipelineDesc& desc) {
    if (!shaders_.Valid(desc.vertexShader.id) ||
        !shaders_.Valid(desc.fragmentShader.id)) {
        KL_ERR("RHI", "Invalid shaders for pipeline creation");
        return {0};
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shaders_.Get(desc.vertexShader.id).shader);
    glAttachShader(program, shaders_.Get(desc.fragmentShader.id).shader);
    glLinkProgram(program);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char info[512];
        glGetProgramInfoLog(program, sizeof(info), nullptr, info);
        KL_ERR("RHI", "Program link error: %s", info);
        glDeleteProgram(program);
        return {0};
    }

    // Bind the push-constant UBO block if present
    GLuint blockIdx = glGetUniformBlockIndex(program, "PushConstants");
    bool hasPCBlock = (blockIdx != GL_INVALID_INDEX);
    if (hasPCBlock)
        glUniformBlockBinding(program, blockIdx, kPushConstantBinding);

    PipelineSlot slot{};
    slot.program      = program;
    slot.topology     = desc.topology;
    slot.rasterizer   = desc.rasterizer;
    slot.depthStencil = desc.depthStencil;
    slot.blend        = desc.blend;
    slot.attrCount    = desc.vertexAttrCount;
    slot.vertexStride = desc.vertexStride;
    slot.hasPushConstantBlock = hasPCBlock;
    std::memcpy(slot.attrs, desc.vertexAttrs,
                desc.vertexAttrCount * sizeof(RHIVertexAttr));

    // Store material param names for name-based uniform bridging.
    for (uint32_t i = 0; i < desc.materialParamCount; ++i) {
        PipelineSlot::MatParam mp;
        mp.name = desc.materialParams[i].name ? desc.materialParams[i].name : "";
        mp.type = desc.materialParams[i].type;
        slot.materialParams.push_back(std::move(mp));
    }

    return {pipelines_.Alloc(slot)};
}

void OpenGLRHIDevice::DestroyPipeline(RHIPipeline handle) {
    if (!pipelines_.Valid(handle.id)) return;
    auto& s = pipelines_.Get(handle.id);
    if (s.program) glDeleteProgram(s.program);
    if (boundPipeline_ == handle.id) { boundPipeline_ = 0; boundPipelinePtr_ = nullptr; }
    pipelines_.Free(handle.id);
}

// -- Render pass -----------------------------------------------------

RHIRenderPass OpenGLRHIDevice::CreateRenderPass(const RHIRenderPassDesc& desc) {
    RenderPassSlot slot{};
    slot.desc = desc;
    return {renderPasses_.Alloc(slot)};
}

void OpenGLRHIDevice::DestroyRenderPass(RHIRenderPass handle) {
    if (!renderPasses_.Valid(handle.id)) return;
    renderPasses_.Free(handle.id);
}

// -- Framebuffer -----------------------------------------------------

RHIFramebuffer OpenGLRHIDevice::CreateFramebuffer(
        RHIRenderPass /*pass*/,
        const RHITexture* colorAttachments, uint32_t colorCount,
        RHITexture depthAttachment,
        uint32_t width, uint32_t height) {
    FramebufferSlot slot{};
    slot.width  = width;
    slot.height = height;

    glGenFramebuffers(1, &slot.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, slot.fbo);

    for (uint32_t i = 0; i < colorCount; ++i) {
        if (!textures_.Valid(colorAttachments[i].id)) continue;
        auto& tex = textures_.Get(colorAttachments[i].id);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                               GL_TEXTURE_2D, tex.texture, 0);
    }

    if (depthAttachment.IsValid() && textures_.Valid(depthAttachment.id)) {
        auto& dtex = textures_.Get(depthAttachment.id);
        GLenum attach = GL_DEPTH_ATTACHMENT;
        if (dtex.pixelFormat == GL_DEPTH_STENCIL)
            attach = GL_DEPTH_STENCIL_ATTACHMENT;
        glFramebufferTexture2D(GL_FRAMEBUFFER, attach,
                               GL_TEXTURE_2D, dtex.texture, 0);
    }

    if (colorCount > 0) {
        GLenum drawBufs[RHIRenderPassDesc::kMaxColorAttachments];
        for (uint32_t i = 0; i < colorCount; ++i)
            drawBufs[i] = GL_COLOR_ATTACHMENT0 + i;
        glDrawBuffers(static_cast<GLsizei>(colorCount), drawBufs);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        KL_ERR("RHI", "Framebuffer incomplete: 0x%x", status);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return {framebuffers_.Alloc(slot)};
}

void OpenGLRHIDevice::DestroyFramebuffer(RHIFramebuffer handle) {
    if (!framebuffers_.Valid(handle.id)) return;
    auto& s = framebuffers_.Get(handle.id);
    if (s.fbo) glDeleteFramebuffers(1, &s.fbo);
    framebuffers_.Free(handle.id);
}

// -- Data transfer ---------------------------------------------------

void OpenGLRHIDevice::UpdateBuffer(RHIBuffer handle, const void* data,
                                    size_t size, size_t offset) {
    if (!buffers_.Valid(handle.id)) return;
    auto& s = buffers_.Get(handle.id);
    glBindBuffer(s.target, s.buffer);
    glBufferSubData(s.target, static_cast<GLintptr>(offset),
                    static_cast<GLsizeiptr>(size), data);
    glBindBuffer(s.target, 0);

    // Maintain CPU shadow for uniform bridging
    size_t needed = offset + size;
    if (s.shadow.size() < needed)
        s.shadow.resize(needed, 0);
    std::memcpy(s.shadow.data() + offset, data, size);
}

void OpenGLRHIDevice::UpdateTexture(RHITexture handle, const void* data,
                                     size_t /*dataSize*/,
                                     uint32_t width, uint32_t height) {
    if (!textures_.Valid(handle.id)) return;
    auto& s = textures_.Get(handle.id);

    // For single-channel formats (R8), set alignment to 1 so rows
    // aren't padded to a 4-byte boundary.
    GLint prevAlignment = 4;
    if (s.pixelFormat == GL_RED) {
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    glBindTexture(GL_TEXTURE_2D, s.texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    static_cast<GLsizei>(width),
                    static_cast<GLsizei>(height),
                    s.pixelFormat, s.pixelType, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (s.pixelFormat == GL_RED)
        glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlignment);
}

void OpenGLRHIDevice::UpdateTextureRegion(RHITexture handle, const void* data,
                                           size_t /*dataSize*/,
                                           uint32_t x, uint32_t y,
                                           uint32_t w, uint32_t h) {
    if (!textures_.Valid(handle.id) || w == 0 || h == 0) return;
    auto& s = textures_.Get(handle.id);

    GLint prevAlignment = 4;
    if (s.pixelFormat == GL_RED) {
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    glBindTexture(GL_TEXTURE_2D, s.texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    static_cast<GLint>(x), static_cast<GLint>(y),
                    static_cast<GLsizei>(w), static_cast<GLsizei>(h),
                    s.pixelFormat, s.pixelType, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (s.pixelFormat == GL_RED)
        glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlignment);
}

void* OpenGLRHIDevice::MapBuffer(RHIBuffer handle) {
    if (!buffers_.Valid(handle.id)) return nullptr;
    auto& s = buffers_.Get(handle.id);
    if (!s.hostVisible) return nullptr;
    glBindBuffer(s.target, s.buffer);
    s.mapped = glMapBufferRange(s.target, 0, s.size,
                                GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
    glBindBuffer(s.target, 0);
    return s.mapped;
}

void OpenGLRHIDevice::UnmapBuffer(RHIBuffer handle) {
    if (!buffers_.Valid(handle.id)) return;
    auto& s = buffers_.Get(handle.id);
    if (!s.mapped) return;
    glBindBuffer(s.target, s.buffer);
    glUnmapBuffer(s.target);
    glBindBuffer(s.target, 0);
    s.mapped = nullptr;
}

// -- Fixed-function state helpers ------------------------------------

void OpenGLRHIDevice::ApplyRasterizerState(const RHIRasterizerState& rs) {
    // Cull mode
    if (rs.cullMode == RHICullMode::None) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        switch (rs.cullMode) {
            case RHICullMode::Front:        glCullFace(GL_FRONT); break;
            case RHICullMode::Back:         glCullFace(GL_BACK); break;
            case RHICullMode::FrontAndBack: glCullFace(GL_FRONT_AND_BACK); break;
            default: break;
        }
    }

    // Winding order
    glFrontFace(rs.frontFace == RHIFrontFace::CounterClockwise ? GL_CCW : GL_CW);

    // Polygon mode
    switch (rs.polygonMode) {
        case RHIPolygonMode::Line:  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
        case RHIPolygonMode::Point: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
        default:                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
    }

    // Depth clamp
    if (rs.depthClamp)
        glEnable(GL_DEPTH_CLAMP);
    else
        glDisable(GL_DEPTH_CLAMP);
}

void OpenGLRHIDevice::ApplyDepthStencilState(const RHIDepthStencilState& ds) {
    if (ds.depthTest)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    glDepthMask(ds.depthWrite ? GL_TRUE : GL_FALSE);
    glDepthFunc(ToGLCompareFunc(ds.depthOp));

    if (ds.stencilTest)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);
}

void OpenGLRHIDevice::ApplyBlendState(const RHIBlendState& bs) {
    if (bs.enabled) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(ToGLBlendFactor(bs.srcColor),
                            ToGLBlendFactor(bs.dstColor),
                            ToGLBlendFactor(bs.srcAlpha),
                            ToGLBlendFactor(bs.dstAlpha));
        glBlendEquationSeparate(ToGLBlendOp(bs.colorOp),
                                ToGLBlendOp(bs.alphaOp));
    } else {
        glDisable(GL_BLEND);
    }
}

void OpenGLRHIDevice::SetupVertexAttributes() {
    if (!boundPipelinePtr_ || !boundVertexBufPtr_) return;

    auto& pipe = *boundPipelinePtr_;
    auto& vb   = *boundVertexBufPtr_;

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vb.buffer);

    // Build a bitmask of the attribute slots used by this pipeline.
    // Disable only those slots that were enabled before but are no
    // longer needed; enable + configure the rest. This avoids
    // unconditionally issuing 8 glDisableVertexAttribArray calls per
    // draw (most pipelines use 2-4 slots).
    uint32_t newMask = 0;
    for (uint32_t i = 0; i < pipe.attrCount; ++i) {
        const uint32_t loc = pipe.attrs[i].location;
        if (loc < 32u) newMask |= (1u << loc);
    }

    const uint32_t toDisable = lastAttrMask_ & ~newMask;
    if (toDisable) {
        for (uint32_t i = 0; i < 32u; ++i) {
            if (toDisable & (1u << i))
                glDisableVertexAttribArray(i);
        }
    }

    for (uint32_t i = 0; i < pipe.attrCount; ++i) {
        auto& attr = pipe.attrs[i];
        GLint  comps = VertexAttrComponentCount(attr.format);
        GLenum type  = VertexAttrGLType(attr.format);
        GLboolean normalize = (type == GL_UNSIGNED_BYTE) ? GL_TRUE : GL_FALSE;

        // Always re-enable + re-configure: vertex stride, format, or
        // base offset can vary even when the same slot was used before.
        glEnableVertexAttribArray(attr.location);
        glVertexAttribPointer(
            attr.location, comps, type, normalize,
            static_cast<GLsizei>(pipe.vertexStride),
            reinterpret_cast<const void*>(
                static_cast<uintptr_t>(attr.offset + boundVBOffset_)));
    }

    lastAttrMask_     = newMask;
    vertexStateDirty_ = false;
}

// -- Command recording -----------------------------------------------

void OpenGLRHIDevice::BeginFrame() {
    // OpenGL is immediate-mode; nothing to acquire.
}

void OpenGLRHIDevice::BeginRenderPass(RHIRenderPass pass,
                                       RHIFramebuffer framebuffer,
                                       const RHIClearValue& clear) {
    GLuint fbo = 0;
    uint32_t w = 0, h = 0;

    if (framebuffers_.Valid(framebuffer.id)) {
        auto& fb = framebuffers_.Get(framebuffer.id);
        fbo = fb.fbo;
        w   = fb.width;
        h   = fb.height;
    } else {
        // Default framebuffer
        display_->GetWindowSize(w, h);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));

    GLbitfield clearBits = 0;
    if (renderPasses_.Valid(pass.id)) {
        auto& rp = renderPasses_.Get(pass.id);
        for (uint32_t i = 0; i < rp.desc.colorAttachmentCount; ++i) {
            if (rp.desc.colorAttachments[i].loadOp == RHILoadOp::Clear)
                clearBits |= GL_COLOR_BUFFER_BIT;
        }
        if (rp.desc.hasDepth && rp.desc.depthLoadOp == RHILoadOp::Clear)
            clearBits |= GL_DEPTH_BUFFER_BIT;
    } else {
        clearBits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
    }

    if (clearBits) {
        // Ensure depth/color writes are enabled before clearing.
        // glClear respects glDepthMask/glColorMask; a prior pipeline
        // (e.g. overlay) may have left depth writes disabled.
        if (clearBits & GL_DEPTH_BUFFER_BIT)
            glDepthMask(GL_TRUE);
        if (clearBits & GL_COLOR_BUFFER_BIT)
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        glClearColor(clear.color[0], clear.color[1],
                     clear.color[2], clear.color[3]);
        glClearDepthf(clear.depth);
        glClearStencil(clear.stencil);
        glClear(clearBits);
    }
}

void OpenGLRHIDevice::EndRenderPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRHIDevice::BindPipeline(RHIPipeline pipeline) {
    if (!pipelines_.Valid(pipeline.id)) return;
    auto& p = pipelines_.Get(pipeline.id);

    glUseProgram(p.program);
    ApplyRasterizerState(p.rasterizer);
    ApplyDepthStencilState(p.depthStencil);
    ApplyBlendState(p.blend);

    boundPipeline_    = pipeline.id;
    boundPipelinePtr_ = &p;
    vertexStateDirty_ = true;

    // Re-apply bridge uniforms on the new program for all tracked UBO bindings.
    // glUniform* state is per-program, so switching programs loses them.
    for (uint32_t set = 0; set < kMaxBindingSets; ++set) {
        for (uint32_t binding = 0; binding < kMaxBindingsPerSet; ++binding) {
            auto& tb = trackedBindings_[set * kMaxBindingsPerSet + binding];
            if (!tb.active || !buffers_.Valid(tb.bufferId)) continue;

            auto& s = buffers_.Get(tb.bufferId);
            if (s.shadow.empty()) continue;

            size_t len = std::min(tb.range, s.shadow.size() - tb.offset);
            const uint8_t* data = s.shadow.data() + tb.offset;

            if (set == 0 && binding == 0)      BridgeTransformUniforms(p, data, len);
            else if (set == 0 && binding == 1) BridgeLightUniforms(p, data, len);
            else if (set == 0 && binding == 2) BridgeSceneUniforms(p, data, len);
            else if (set == 0 && binding == 3) BridgeAudioUniforms(p, data, len);
            else if (set == 1 && binding == 0) BridgeMaterialUniforms(p, data, len);
        }
    }
}

void OpenGLRHIDevice::BindVertexBuffer(RHIBuffer buffer,
                                        uint32_t /*binding*/, uint64_t offset) {
    boundVertexBuf_    = buffer.id;
    boundVertexBufPtr_ = buffers_.GetPtr(buffer.id);
    boundVBOffset_     = offset;
    vertexStateDirty_  = true;
}

void OpenGLRHIDevice::BindIndexBuffer(RHIBuffer buffer,
                                       bool is32Bit, uint64_t offset) {
    boundIndexBuf_    = buffer.id;
    boundIndexBufPtr_ = buffers_.GetPtr(buffer.id);
    indexIs32Bit_     = is32Bit;
    boundIBOffset_    = offset;

    if (boundIndexBufPtr_) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boundIndexBufPtr_->buffer);
    }
}

void OpenGLRHIDevice::BindUniformBuffer(RHIBuffer buffer, uint32_t set,
                                         uint32_t binding,
                                         size_t offset, size_t range) {
    if (!buffers_.Valid(buffer.id)) return;
    auto& s = buffers_.Get(buffer.id);
    size_t r = (range == 0) ? static_cast<size_t>(s.size) : range;
    glBindBufferRange(GL_UNIFORM_BUFFER, binding, s.buffer,
                      static_cast<GLintptr>(offset),
                      static_cast<GLsizeiptr>(r));

    // Track this binding for re-applying bridge uniforms on program switch (C3: O(1) direct index)
    if (set < kMaxBindingSets && binding < kMaxBindingsPerSet) {
        auto& tb = trackedBindings_[set * kMaxBindingsPerSet + binding];
        tb.bufferId = buffer.id;
        tb.offset   = offset;
        tb.range    = r;
        tb.active   = true;
    }

    // Bridge: also set individual uniforms for shaders that don't use UBO blocks.
    if (!boundPipelinePtr_ || s.shadow.empty()) return;
    auto& pipe = *boundPipelinePtr_;

    const uint8_t* data = s.shadow.data() + offset;
    size_t len = std::min(r, s.shadow.size() - offset);

    if (set == 0 && binding == 0)      BridgeTransformUniforms(pipe, data, len);
    else if (set == 0 && binding == 1)  BridgeLightUniforms(pipe, data, len);
    else if (set == 0 && binding == 2)  BridgeSceneUniforms(pipe, data, len);
    else if (set == 0 && binding == 3)  BridgeAudioUniforms(pipe, data, len);
    else if (set == 1 && binding == 0)  BridgeMaterialUniforms(pipe, data, len);
}

void OpenGLRHIDevice::BindTexture(RHITexture texture,
                                   uint32_t /*set*/, uint32_t binding) {
    if (!textures_.Valid(texture.id)) return;
    auto& s = textures_.Get(texture.id);
    glActiveTexture(GL_TEXTURE0 + binding);
    glBindTexture(GL_TEXTURE_2D, s.texture);
    glBindSampler(binding, s.sampler);
}

void OpenGLRHIDevice::SetViewport(const RHIViewport& vp) {
    glViewport(static_cast<GLint>(vp.x), static_cast<GLint>(vp.y),
               static_cast<GLsizei>(vp.width), static_cast<GLsizei>(vp.height));
    glDepthRangef(vp.minDepth, vp.maxDepth);
}

void OpenGLRHIDevice::SetScissor(const RHIScissor& sc) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(sc.x, sc.y,
              static_cast<GLsizei>(sc.width),
              static_cast<GLsizei>(sc.height));
}

void OpenGLRHIDevice::PushConstants(RHIShaderStage /*stages*/,
                                     const void* data,
                                     uint32_t size, uint32_t offset) {
    // Always update the UBO for programs that use a PushConstants block
    glBindBuffer(GL_UNIFORM_BUFFER, pushConstantUBO_);
    glBufferSubData(GL_UNIFORM_BUFFER,
                    static_cast<GLintptr>(offset),
                    static_cast<GLsizeiptr>(size), data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, kPushConstantBinding, pushConstantUBO_);

    // Fallback: if the bound program uses plain uniforms instead of a UBO
    // block, set matching uniforms directly.  This handles shaders written
    // with 'uniform vec2 u_viewport' rather than 'layout(std140) uniform
    // PushConstants { ... }'.
    if (!boundPipelinePtr_) return;
    auto& pipe = *boundPipelinePtr_;
    if (pipe.hasPushConstantBlock) return;

    GLuint prog = pipe.program;
    const auto* raw = static_cast<const uint8_t*>(data);

    // UI push constants: { vec2 viewport (0), int useTexture (8) }
    if (offset == 0 && size >= 8) {
        GLint loc = glGetUniformLocation(prog, "u_viewport");
        if (loc >= 0) {
            float vw, vh;
            std::memcpy(&vw, raw + 0, sizeof(float));
            std::memcpy(&vh, raw + 4, sizeof(float));
            glUniform2f(loc, vw, vh);
        }
    }
    if (offset == 0 && size >= 12) {
        GLint loc = glGetUniformLocation(prog, "u_useTexture");
        if (loc >= 0) {
            int32_t val;
            std::memcpy(&val, raw + 8, sizeof(int32_t));
            glUniform1i(loc, val);
        }
    }
}

void OpenGLRHIDevice::Draw(uint32_t vertexCount, uint32_t instanceCount,
                            uint32_t firstVertex, uint32_t firstInstance) {
    if (vertexStateDirty_) SetupVertexAttributes();
    if (!boundPipelinePtr_) return;

    GLenum mode = ToGLTopology(boundPipelinePtr_->topology);

    if (instanceCount <= 1 && firstInstance == 0) {
        glDrawArrays(mode, static_cast<GLint>(firstVertex),
                     static_cast<GLsizei>(vertexCount));
    } else {
        glDrawArraysInstancedBaseInstance(
            mode, static_cast<GLint>(firstVertex),
            static_cast<GLsizei>(vertexCount),
            static_cast<GLsizei>(instanceCount), firstInstance);
    }
}

void OpenGLRHIDevice::DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                   uint32_t firstIndex, int32_t vertexOffset,
                                   uint32_t firstInstance) {
    if (vertexStateDirty_) SetupVertexAttributes();
    if (!boundPipelinePtr_) return;

    GLenum mode  = ToGLTopology(boundPipelinePtr_->topology);
    GLenum itype = indexIs32Bit_ ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    size_t isize = indexIs32Bit_ ? sizeof(uint32_t) : sizeof(uint16_t);
    const void* offset = reinterpret_cast<const void*>(
        boundIBOffset_ + firstIndex * isize);

    if (instanceCount <= 1 && firstInstance == 0 && vertexOffset == 0) {
        glDrawElements(mode, static_cast<GLsizei>(indexCount),
                       itype, offset);
    } else {
        glDrawElementsInstancedBaseVertexBaseInstance(
            mode, static_cast<GLsizei>(indexCount), itype, offset,
            static_cast<GLsizei>(instanceCount),
            vertexOffset, firstInstance);
    }
}

void OpenGLRHIDevice::EndFrame() {
    glFlush();
}

void OpenGLRHIDevice::Present() {
    display_->SwapOnly();
}

// -- Swapchain -------------------------------------------------------

void OpenGLRHIDevice::GetSwapchainSize(uint32_t& outWidth,
                                        uint32_t& outHeight) const {
    display_->GetWindowSize(outWidth, outHeight);
}

void OpenGLRHIDevice::OnResize(uint32_t width, uint32_t height) {
    glViewport(0, 0, static_cast<GLsizei>(width),
               static_cast<GLsizei>(height));
}

// -- Uniform bridge: UBO data -> individual glUniform* calls -----------
//
// The unified RenderPipeline delivers all data via UBO binding points,
// but KSL-generated GLSL shaders use old-style `uniform mat4 u_model;`
// declarations.  These bridge functions extract data from the CPU shadow
// copy of each UBO and set matching individual uniforms on the bound
// program.  If the shader uses proper UBO blocks instead, the
// glGetUniformLocation calls return -1 and the calls are harmless.

void OpenGLRHIDevice::BridgeTransformUniforms(PipelineSlot& pipe,
                                               const uint8_t* data,
                                               size_t len) {
    // TransformUBO layout (std140):
    //   offset   0: mat4 model          (64 bytes)
    //   offset  64: mat4 view           (64 bytes)
    //   offset 128: mat4 projection     (64 bytes)
    //   offset 192: vec4 cameraPos      (16 bytes) -- xyz + padding
    //   offset 208: mat4 inverseViewProj(64 bytes)
    const auto* f = reinterpret_cast<const float*>(data);
    GLuint prog = pipe.program;
    auto& L = pipe.stdLocs;

    // B3: lazy-cache locations (-2 = unqueried, -1 = absent, >=0 = valid).
    if (L.model == -2)            L.model           = glGetUniformLocation(prog, "u_model");
    if (L.view == -2)             L.view            = glGetUniformLocation(prog, "u_view");
    if (L.projection == -2)       L.projection      = glGetUniformLocation(prog, "u_projection");
    if (L.cameraPos == -2)        L.cameraPos       = glGetUniformLocation(prog, "u_cameraPos");
    if (L.inverseViewProj == -2)  L.inverseViewProj = glGetUniformLocation(prog, "u_inverseViewProj");

    if (len >=  64 && L.model           >= 0) glUniformMatrix4fv(L.model,           1, GL_FALSE, f + 0);
    if (len >= 128 && L.view            >= 0) glUniformMatrix4fv(L.view,            1, GL_FALSE, f + 16);
    if (len >= 192 && L.projection      >= 0) glUniformMatrix4fv(L.projection,      1, GL_FALSE, f + 32);
    if (len >= 208 && L.cameraPos       >= 0) glUniform3fv      (L.cameraPos,       1,           f + 48);
    if (len >= 272 && L.inverseViewProj >= 0) glUniformMatrix4fv(L.inverseViewProj, 1, GL_FALSE, f + 52);
}

void OpenGLRHIDevice::BridgeSceneUniforms(PipelineSlot& pipe,
                                            const uint8_t* data,
                                            size_t len) {
    // SceneUBO layout (std140): int lightCount + 12 bytes padding
    if (len < 4) return;
    auto& L = pipe.stdLocs;
    if (L.lightCount == -2) L.lightCount = glGetUniformLocation(pipe.program, "u_lightCount");
    if (L.lightCount >= 0) {
        int lightCount;
        std::memcpy(&lightCount, data, sizeof(int));
        glUniform1i(L.lightCount, lightCount);
    }
}

void OpenGLRHIDevice::BridgeLightUniforms(PipelineSlot& pipe,
                                            const uint8_t* data,
                                            size_t len) {
    // GPULight layout (std140, 48 bytes each).
    static constexpr size_t kLightStride = 48;
    static constexpr int kMaxLights = 16;

    int count = static_cast<int>(len / kLightStride);
    if (count > kMaxLights) count = kMaxLights;

    GLuint prog = pipe.program;
    auto& L = pipe.stdLocs;

    for (int i = 0; i < count; ++i) {
        const auto* l = reinterpret_cast<const float*>(data + i * kLightStride);

        // B3: lazy-cache per-light array element locations.
        if (L.lightPos[i] == -2) {
            char name[64];
            std::snprintf(name, sizeof(name), "u_lights[%d].position", i);
            L.lightPos[i] = glGetUniformLocation(prog, name);
            std::snprintf(name, sizeof(name), "u_lights[%d].color", i);
            L.lightCol[i] = glGetUniformLocation(prog, name);
            std::snprintf(name, sizeof(name), "u_lights[%d].intensity", i);
            L.lightInt[i] = glGetUniformLocation(prog, name);
            std::snprintf(name, sizeof(name), "u_lights[%d].falloff", i);
            L.lightFalloff[i] = glGetUniformLocation(prog, name);
            std::snprintf(name, sizeof(name), "u_lights[%d].curve", i);
            L.lightCurve[i] = glGetUniformLocation(prog, name);
        }

        if (L.lightPos[i]     >= 0) glUniform3fv(L.lightPos[i], 1, l + 0);
        if (L.lightCol[i]     >= 0) glUniform3fv(L.lightCol[i], 1, l + 4);
        if (L.lightInt[i]     >= 0) glUniform1f (L.lightInt[i],    l[3]);
        if (L.lightFalloff[i] >= 0) glUniform1f (L.lightFalloff[i],l[7]);
        if (L.lightCurve[i]   >= 0) glUniform1f (L.lightCurve[i],  l[8]);
    }
}

void OpenGLRHIDevice::BridgeAudioUniforms(PipelineSlot& /*pipe*/,
                                            const uint8_t* data,
                                            size_t len) {
    // Audio SSBO is currently a 16-byte dummy.
    (void)data; (void)len;
}

void OpenGLRHIDevice::BridgeMaterialUniforms(PipelineSlot& pipe,
                                              const uint8_t* data,
                                              size_t len) {
    if (len == 0) return;

    // Use name-based path when param metadata is available.
    if (!pipe.materialParams.empty()) {
            size_t offset = 0;
            for (auto& mp : pipe.materialParams) {
                // Compute std140 alignment and data size from the KSL type.
                size_t align = 4, sz = 4;
                GLenum glType = GL_FLOAT;
                switch (mp.type) {
                    case 0: /* Float */ align = 4;  sz = 4;  glType = GL_FLOAT;      break;
                    case 1: /* Int */   align = 4;  sz = 4;  glType = GL_INT;        break;
                    case 2: /* Bool */  align = 4;  sz = 4;  glType = GL_INT;        break;
                    case 3: /* Vec2 */  align = 8;  sz = 8;  glType = GL_FLOAT_VEC2; break;
                    case 4: /* Vec3 */  align = 16; sz = 12; glType = GL_FLOAT_VEC3; break;
                    case 5: /* Vec4 */  align = 16; sz = 16; glType = GL_FLOAT_VEC4; break;
                    default: continue;
                }

                offset = (offset + align - 1) & ~(align - 1);
                if (offset + sz > len) break;

                // B3: lazy-cache the uniform location per material param.
                if (mp.cachedLoc == -2) {
                    std::string uniformName = "u_" + mp.name;
                    mp.cachedLoc = glGetUniformLocation(pipe.program, uniformName.c_str());
                }
                GLint loc = mp.cachedLoc;
                if (loc >= 0) {
                    const float* fptr = reinterpret_cast<const float*>(data + offset);
                    const int*   iptr = reinterpret_cast<const int*>(data + offset);
                    switch (glType) {
                        case GL_FLOAT:      glUniform1fv(loc, 1, fptr); break;
                        case GL_INT:        glUniform1iv(loc, 1, iptr); break;
                        case GL_FLOAT_VEC2: glUniform2fv(loc, 1, fptr); break;
                        case GL_FLOAT_VEC3: glUniform3fv(loc, 1, fptr); break;
                        case GL_FLOAT_VEC4: glUniform4fv(loc, 1, fptr); break;
                        default: break;
                    }
                }

                offset += sz;
            }
            return;
    }

    // Fallback: no metadata -- enumerate active uniforms and sort by location.
    GLuint prog = pipe.program;
    GLint activeCount = 0;
    glGetProgramiv(prog, GL_ACTIVE_UNIFORMS, &activeCount);

    static const char* kSkipPrefixes[] = {
        "u_model", "u_view", "u_projection", "u_cameraPos",
        "u_inverseViewProj", "u_lights[", "u_lightCount",
        "u_texture", "u_shaderID", "u_audioSamples", "u_sampleCount",
        "u_time", "u_resolution", nullptr
    };

    struct UniformInfo {
        GLint location;
        GLenum type;
        GLint arraySize;
        char name[128];
    };
    std::vector<UniformInfo> candidates;

    for (GLint i = 0; i < activeCount; ++i) {
        UniformInfo info{};
        GLsizei nameLen = 0;
        glGetActiveUniform(prog, static_cast<GLuint>(i), sizeof(info.name),
                           &nameLen, &info.arraySize, &info.type, info.name);
        info.location = glGetUniformLocation(prog, info.name);
        if (info.location < 0) continue;

        bool skip = false;
        for (const char** p = kSkipPrefixes; *p; ++p) {
            if (std::strncmp(info.name, *p, std::strlen(*p)) == 0) {
                skip = true;
                break;
            }
        }
        if (skip) continue;

        candidates.push_back(info);
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const UniformInfo& a, const UniformInfo& b) {
                  return a.location < b.location;
              });

    size_t offset = 0;
    for (auto& u : candidates) {
        size_t align = 4, sz = 4;
        switch (u.type) {
            case GL_FLOAT:      align = 4;  sz = 4;  break;
            case GL_FLOAT_VEC2: align = 8;  sz = 8;  break;
            case GL_FLOAT_VEC3: align = 16; sz = 12; break;
            case GL_FLOAT_VEC4: align = 16; sz = 16; break;
            case GL_INT:        align = 4;  sz = 4;  break;
            case GL_FLOAT_MAT4: align = 16; sz = 64; break;
            default: continue;
        }

        offset = (offset + align - 1) & ~(align - 1);
        if (offset + sz > len) break;

        const float* fptr = reinterpret_cast<const float*>(data + offset);
        const int*   iptr = reinterpret_cast<const int*>(data + offset);

        switch (u.type) {
            case GL_FLOAT:      glUniform1fv(u.location, 1, fptr);                   break;
            case GL_FLOAT_VEC2: glUniform2fv(u.location, 1, fptr);                   break;
            case GL_FLOAT_VEC3: glUniform3fv(u.location, 1, fptr);                   break;
            case GL_FLOAT_VEC4: glUniform4fv(u.location, 1, fptr);                   break;
            case GL_INT:        glUniform1iv(u.location, 1, iptr);                   break;
            case GL_FLOAT_MAT4: glUniformMatrix4fv(u.location, 1, GL_FALSE, fptr);   break;
            default: break;
        }

        offset += sz;
    }
}

// ---------------------------------------------------------------------------
// Timestamp queries (GL_ARB_timer_query / GL 3.3 core)
// ---------------------------------------------------------------------------

void OpenGLRHIDevice::ResetTimestamps(uint32_t maxQueries) {
    if (!tsSupported_) return;

    // Swap current - prev so ReadTimestamps can read last frame's results
    std::swap(tsQueries_, tsQueriesPrev_);
    tsPrevCount_  = tsWriteCount_;
    tsWriteCount_ = 0;

    // Ensure we have enough query objects for this frame
    if (tsQueries_.size() < maxQueries) {
        size_t oldSize = tsQueries_.size();
        tsQueries_.resize(maxQueries, 0);
        glGenQueries(static_cast<GLsizei>(maxQueries - oldSize),
                     tsQueries_.data() + oldSize);
    }
}

void OpenGLRHIDevice::WriteTimestamp(uint32_t index) {
    if (!tsSupported_ || index >= tsQueries_.size()) return;
    glQueryCounter(tsQueries_[index], GL_TIMESTAMP);
    if (index >= tsWriteCount_) tsWriteCount_ = index + 1;
}

bool OpenGLRHIDevice::ReadTimestamps(uint64_t* out, uint32_t count) {
    if (!tsSupported_ || tsPrevCount_ == 0) return false;
    uint32_t toRead = (count < tsPrevCount_) ? count : tsPrevCount_;

    for (uint32_t i = 0; i < toRead; ++i) {
        GLint available = GL_FALSE;
        glGetQueryObjectiv(tsQueriesPrev_[i],
                           GL_QUERY_RESULT_AVAILABLE, &available);
        if (!available) return false;
    }
    for (uint32_t i = 0; i < toRead; ++i) {
        GLuint64 val = 0;
        glGetQueryObjectui64v(tsQueriesPrev_[i], GL_QUERY_RESULT, &val);
        out[i] = static_cast<uint64_t>(val);
    }
    return true;
}

double OpenGLRHIDevice::GetTimestampPeriod() const {
    return tsSupported_ ? 1.0 : 0.0; // GL timestamps are already in nanoseconds
}

void OpenGLRHIDevice::WaitIdle() {
    glFinish();
}

} // namespace koilo::rhi
