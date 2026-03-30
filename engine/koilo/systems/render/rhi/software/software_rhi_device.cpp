// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file software_rhi_device.cpp
 * @brief Software (CPU) implementation of IRHIDevice.
 *
 * All Create/Destroy/Update methods are fully functional 
 * (CPU-side storage).  Draw() calls are logged but do not
 * rasterize yet (SW-2 will add that).
 *
 * @date 03/29/2026
 * @author Coela Can't
 */
#include "software_rhi_device.hpp"
#include <koilo/kernel/logging/log.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace koilo::rhi {

// -- Lifecycle -------------------------------------------------------

SoftwareRHIDevice::~SoftwareRHIDevice() {
    if (initialized_) Shutdown();
}

bool SoftwareRHIDevice::Initialize() {
    if (initialized_) return true;
    initialized_ = true;

    // Create a dummy swapchain render pass (pipelines that target the
    // screen need a valid render pass handle for compatibility).
    RHIRenderPassDesc swDesc;
    swDesc.colorAttachmentCount = 1;
    swDesc.colorAttachments[0].format  = RHIFormat::RGBA8_Unorm;
    swDesc.colorAttachments[0].loadOp  = RHILoadOp::Clear;
    swDesc.colorAttachments[0].storeOp = RHIStoreOp::Store;
    swDesc.hasDepth      = false;
    swapchainRenderPass_ = CreateRenderPass(swDesc);

    KL_LOG("SoftwareRHI", "Initialized");
    return true;
}

void SoftwareRHIDevice::Shutdown() {
    if (!initialized_) return;

    buffers_.Clear();
    textures_.Clear();
    shaders_.Clear();
    pipelines_.Clear();
    renderPasses_.Clear();
    framebuffers_.Clear();

    swapchainPixels_.clear();
    swapchainWidth_  = 0;
    swapchainHeight_ = 0;
    swapchainRenderPass_ = {};
    initialized_ = false;

    KL_LOG("SoftwareRHI", "Shut down");
}

// -- Capabilities ----------------------------------------------------

bool SoftwareRHIDevice::SupportsFeature(RHIFeature /*feature*/) const {
    return false; // No GPU features available
}

RHILimits SoftwareRHIDevice::GetLimits() const {
    RHILimits lim;
    lim.maxTextureSize       = 4096;
    lim.maxFramebufferWidth  = 4096;
    lim.maxFramebufferHeight = 4096;
    lim.maxColorAttachments  = 1;
    lim.maxVertexAttributes  = 8;
    lim.maxVertexBindings    = 4;
    lim.maxUniformBufferSize = 65536;
    lim.maxStorageBufferSize = 16ULL * 1024 * 1024;
    lim.maxBoundDescriptorSets = 4;
    lim.maxSamplers          = 8;
    lim.totalDeviceMemory    = 0;
    return lim;
}

// -- Resource creation / destruction ---------------------------------

RHIBuffer SoftwareRHIDevice::CreateBuffer(const RHIBufferDesc& desc) {
    BufferSlot slot;
    slot.data.resize(static_cast<size_t>(desc.size), 0);
    slot.usage       = desc.usage;
    slot.hostVisible = desc.hostVisible;
    return {buffers_.Alloc(slot)};
}

void SoftwareRHIDevice::DestroyBuffer(RHIBuffer handle) {
    if (!buffers_.Valid(handle.id)) return;
    buffers_.Free(handle.id);
}

RHITexture SoftwareRHIDevice::CreateTexture(const RHITextureDesc& desc) {
    TextureSlot slot;
    slot.width  = desc.width;
    slot.height = desc.height;
    slot.format = desc.format;
    slot.filter = desc.filter;
    size_t bpp = RHIFormatBytesPerPixel(desc.format);
    if (bpp == 0) bpp = 4; // fallback
    slot.pixels.resize(static_cast<size_t>(desc.width) * desc.height * bpp, 0);
    return {textures_.Alloc(slot)};
}

void SoftwareRHIDevice::DestroyTexture(RHITexture handle) {
    if (!textures_.Valid(handle.id)) return;
    textures_.Free(handle.id);
}

RHIShader SoftwareRHIDevice::CreateShader(RHIShaderStage stage,
                                           const void* code,
                                           size_t codeSize) {
    ShaderSlot slot;
    slot.stage = stage;
    if (code && codeSize > 0) {
        slot.code.resize(codeSize);
        std::memcpy(slot.code.data(), code, codeSize);
    }
    return {shaders_.Alloc(slot)};
}

void SoftwareRHIDevice::DestroyShader(RHIShader handle) {
    if (!shaders_.Valid(handle.id)) return;
    shaders_.Free(handle.id);
}

RHIPipeline SoftwareRHIDevice::CreatePipeline(const RHIPipelineDesc& desc) {
    PipelineSlot slot;
    slot.desc = desc;
    return {pipelines_.Alloc(slot)};
}

void SoftwareRHIDevice::DestroyPipeline(RHIPipeline handle) {
    if (!pipelines_.Valid(handle.id)) return;
    pipelines_.Free(handle.id);
}

RHIRenderPass SoftwareRHIDevice::CreateRenderPass(const RHIRenderPassDesc& desc) {
    RenderPassSlot slot;
    slot.desc = desc;
    return {renderPasses_.Alloc(slot)};
}

void SoftwareRHIDevice::DestroyRenderPass(RHIRenderPass handle) {
    if (!renderPasses_.Valid(handle.id)) return;
    renderPasses_.Free(handle.id);
}

RHIFramebuffer SoftwareRHIDevice::CreateFramebuffer(
    RHIRenderPass pass,
    const RHITexture* colorAttachments,
    uint32_t colorCount,
    RHITexture depthAttachment,
    uint32_t width, uint32_t height)
{
    FramebufferSlot slot;
    slot.renderPass = pass;
    for (uint32_t i = 0; i < colorCount; ++i)
        slot.colorAttachments.push_back(colorAttachments[i]);
    slot.depthAttachment = depthAttachment;
    slot.width  = width;
    slot.height = height;
    return {framebuffers_.Alloc(slot)};
}

void SoftwareRHIDevice::DestroyFramebuffer(RHIFramebuffer handle) {
    if (!framebuffers_.Valid(handle.id)) return;
    framebuffers_.Free(handle.id);
}

// -- Data transfer ---------------------------------------------------

void SoftwareRHIDevice::UpdateBuffer(RHIBuffer handle,
                                      const void* data, size_t size,
                                      size_t offset) {
    if (!buffers_.Valid(handle.id) || !data) return;
    auto& buf = buffers_.Get(handle.id);
    if (offset + size > buf.data.size())
        buf.data.resize(offset + size);
    std::memcpy(buf.data.data() + offset, data, size);
}

void SoftwareRHIDevice::UpdateTexture(RHITexture handle,
                                       const void* data, size_t dataSize,
                                       uint32_t width, uint32_t height) {
    if (!textures_.Valid(handle.id) || !data) return;
    auto& tex = textures_.Get(handle.id);
    tex.width  = width;
    tex.height = height;
    size_t bpp = RHIFormatBytesPerPixel(tex.format);
    if (bpp == 0) bpp = 4;
    size_t expected = static_cast<size_t>(width) * height * bpp;
    size_t copySize = std::min(dataSize, expected);
    if (tex.pixels.size() < expected)
        tex.pixels.resize(expected);
    std::memcpy(tex.pixels.data(), data, copySize);
}

void* SoftwareRHIDevice::MapBuffer(RHIBuffer handle) {
    if (!buffers_.Valid(handle.id)) return nullptr;
    return buffers_.Get(handle.id).data.data();
}

void SoftwareRHIDevice::UnmapBuffer(RHIBuffer /*handle*/) {
    // No-op: CPU memory is always accessible
}

// -- Command recording -----------------------------------------------

void SoftwareRHIDevice::BeginFrame() {
    // Reset per-frame state
    boundPipeline_ = {};
}

void SoftwareRHIDevice::BeginRenderPass(RHIRenderPass pass,
                                         RHIFramebuffer framebuffer,
                                         const RHIClearValue& clear) {
    activeRenderPass_  = pass;
    activeFramebuffer_ = framebuffer;
    renderingToSwapchain_ = false;

    // Reset bound textures to prevent stale bindings from previous passes
    std::memset(boundTextures_, 0, sizeof(boundTextures_));

    if (framebuffers_.Valid(framebuffer.id)) {
        auto& fbo = framebuffers_.Get(framebuffer.id);
        for (auto& colorTex : fbo.colorAttachments) {
            if (!textures_.Valid(colorTex.id)) continue;
            auto& tex = textures_.Get(colorTex.id);
            uint8_t r = static_cast<uint8_t>(clear.color[0] * 255.0f);
            uint8_t g = static_cast<uint8_t>(clear.color[1] * 255.0f);
            uint8_t b = static_cast<uint8_t>(clear.color[2] * 255.0f);
            uint8_t a = static_cast<uint8_t>(clear.color[3] * 255.0f);
            size_t bpp = RHIFormatBytesPerPixel(tex.format);
            if (bpp == 4) {
                uint32_t packed = static_cast<uint32_t>(r)
                                | (static_cast<uint32_t>(g) << 8)
                                | (static_cast<uint32_t>(b) << 16)
                                | (static_cast<uint32_t>(a) << 24);
                uint32_t* px = reinterpret_cast<uint32_t*>(tex.pixels.data());
                size_t count = tex.pixels.size() / 4;
                std::fill(px, px + count, packed);
            }
        }
        if (textures_.Valid(fbo.depthAttachment.id)) {
            auto& dtex = textures_.Get(fbo.depthAttachment.id);
            if (RHIFormatBytesPerPixel(dtex.format) == 4 && RHIFormatIsDepth(dtex.format)) {
                // Treat all 4-byte depth formats as float storage in software
                float* dp = reinterpret_cast<float*>(dtex.pixels.data());
                size_t count = dtex.pixels.size() / sizeof(float);
                std::fill(dp, dp + count, clear.depth);
            } else {
                std::fill(dtex.pixels.begin(), dtex.pixels.end(), 0xFF);
            }
        }
    }
}

void SoftwareRHIDevice::EndRenderPass() {
    activeRenderPass_  = {};
    activeFramebuffer_ = {};
    renderingToSwapchain_ = false;
}

void SoftwareRHIDevice::BindPipeline(RHIPipeline pipeline) {
    boundPipeline_ = pipeline;
}

void SoftwareRHIDevice::BindVertexBuffer(RHIBuffer buffer,
                                          uint32_t binding,
                                          uint64_t offset) {
    if (binding < kMaxBindings) {
        boundVertexBuffers_[binding] = buffer;
        boundVertexOffsets_[binding] = offset;
    }
}

void SoftwareRHIDevice::BindIndexBuffer(RHIBuffer buffer,
                                         bool is32Bit,
                                         uint64_t /*offset*/) {
    boundIndexBuffer_  = buffer;
    boundIndexIs32Bit_ = is32Bit;
}

void SoftwareRHIDevice::BindUniformBuffer(RHIBuffer buffer,
                                           uint32_t set,
                                           uint32_t binding,
                                           size_t offset,
                                           size_t range) {
    if (set < kMaxSets && binding < kMaxBindings) {
        boundUniformBuffers_[set][binding] = buffer;
        boundUniformOffsets_[set][binding]  = offset;
        boundUniformRanges_[set][binding]   = range;
    }
}

void SoftwareRHIDevice::BindTexture(RHITexture texture,
                                     uint32_t set,
                                     uint32_t binding) {
    if (set < kMaxSets && binding < kMaxBindings) {
        boundTextures_[set][binding] = texture;
    }
}

void SoftwareRHIDevice::SetViewport(const RHIViewport& vp) {
    viewport_ = vp;
}

void SoftwareRHIDevice::SetScissor(const RHIScissor& sc) {
    scissor_ = sc;
}

void SoftwareRHIDevice::PushConstants(RHIShaderStage /*stages*/,
                                       const void* /*data*/,
                                       uint32_t /*size*/,
                                       uint32_t /*offset*/) {
    // Not used by current pipeline
}

void SoftwareRHIDevice::Draw(uint32_t vertexCount,
                              uint32_t /*instanceCount*/,
                              uint32_t firstVertex,
                              uint32_t /*firstInstance*/) {
    if (vertexCount == 0) return;
    if (!pipelines_.Valid(boundPipeline_.id)) return;

    // Get vertex data from bound vertex buffer (binding 0)
    RHIBuffer vbo = boundVertexBuffers_[0];
    if (!buffers_.Valid(vbo.id)) return;

    auto& buf = buffers_.Get(vbo.id);
    auto& desc = pipelines_.Get(boundPipeline_.id).desc;
    uint32_t stride = desc.vertexStride;
    if (stride == 0) return;

    const uint8_t* verts = buf.data.data() + boundVertexOffsets_[0];

    ColorTarget target = GetActiveColorTarget();
    if (!target.pixels || target.width == 0 || target.height == 0) return;

    if (desc.topology == RHITopology::LineList) {
        RasterizeLines(verts, vertexCount, firstVertex, stride, desc, target);
    } else if (desc.topology == RHITopology::TriangleList) {
        // Fullscreen quads: 2-attribute, 16-byte stride, no depth
        if (desc.vertexAttrCount == 2 && stride == 16 && !desc.depthStencil.depthTest) {
            BlitFullscreenQuad(verts, vertexCount, firstVertex, stride, target);
        } else if (desc.vertexAttrCount == 5 && stride == 52) {
            // UI quads: 2D screen-space vertices (UIVertex format)
            RasterizeUIQuads(verts, vertexCount, firstVertex, stride, desc, target);
        } else {
            RasterizeTriangles(verts, vertexCount, firstVertex, stride, desc, target);
        }
    }
}

void SoftwareRHIDevice::DrawIndexed(uint32_t /*indexCount*/,
                                     uint32_t /*instanceCount*/,
                                     uint32_t /*firstIndex*/,
                                     int32_t  /*vertexOffset*/,
                                     uint32_t /*firstInstance*/) {
    // Not used by current pipeline
}

void SoftwareRHIDevice::EndFrame() {
    // No-op: no command submission needed
}

void SoftwareRHIDevice::Present() {
    // No-op: the host reads pixels directly via GetSwapchainPixels()
    // and uploads them to the display backend.
}

// -- Swapchain / surface ---------------------------------------------

void SoftwareRHIDevice::BeginSwapchainRenderPass(const RHIClearValue& clear) {
    renderingToSwapchain_ = true;
    activeFramebuffer_ = {};

    // Reset bound textures to prevent stale bindings from previous passes
    std::memset(boundTextures_, 0, sizeof(boundTextures_));

    if (swapchainPixels_.empty()) return;

    uint8_t r = static_cast<uint8_t>(clear.color[0] * 255.0f);
    uint8_t g = static_cast<uint8_t>(clear.color[1] * 255.0f);
    uint8_t b = static_cast<uint8_t>(clear.color[2] * 255.0f);
    uint8_t a = static_cast<uint8_t>(clear.color[3] * 255.0f);

    uint32_t packed = static_cast<uint32_t>(r)
                    | (static_cast<uint32_t>(g) << 8)
                    | (static_cast<uint32_t>(b) << 16)
                    | (static_cast<uint32_t>(a) << 24);
    uint32_t* px = reinterpret_cast<uint32_t*>(swapchainPixels_.data());
    size_t count = swapchainPixels_.size() / 4;
    std::fill(px, px + count, packed);
    std::fill(swapchainDepth_.begin(), swapchainDepth_.end(), clear.depth);
}

RHIRenderPass SoftwareRHIDevice::GetSwapchainRenderPass() const {
    return swapchainRenderPass_;
}

void SoftwareRHIDevice::GetSwapchainSize(uint32_t& outWidth,
                                          uint32_t& outHeight) const {
    outWidth  = swapchainWidth_;
    outHeight = swapchainHeight_;
}

void SoftwareRHIDevice::OnResize(uint32_t width, uint32_t height) {
    if (width == swapchainWidth_ && height == swapchainHeight_) return;
    swapchainWidth_  = width;
    swapchainHeight_ = height;
    size_t pixels = static_cast<size_t>(width) * height;
    swapchainPixels_.resize(pixels * 4, 0);
    swapchainDepth_.resize(pixels, 1.0f);
}

// -- Software-specific API -------------------------------------------

const uint8_t* SoftwareRHIDevice::GetSwapchainPixels() const {
    return swapchainPixels_.empty() ? nullptr : swapchainPixels_.data();
}

// ========================================================================
// SW-2: CPU Rasterization
// ========================================================================

SoftwareRHIDevice::ColorTarget SoftwareRHIDevice::GetActiveColorTarget() {
    ColorTarget t;

    if (renderingToSwapchain_) {
        if (!swapchainPixels_.empty()) {
            t.pixels = swapchainPixels_.data();
            t.depth  = swapchainDepth_.empty() ? nullptr : swapchainDepth_.data();
            t.width  = swapchainWidth_;
            t.height = swapchainHeight_;
        }
        return t;
    }

    if (!framebuffers_.Valid(activeFramebuffer_.id)) return t;
    auto& fbo = framebuffers_.Get(activeFramebuffer_.id);
    t.width  = fbo.width;
    t.height = fbo.height;

    // First color attachment
    if (!fbo.colorAttachments.empty()) {
        RHITexture ct = fbo.colorAttachments[0];
        if (textures_.Valid(ct.id)) {
            auto& tex = textures_.Get(ct.id);
            if (RHIFormatBytesPerPixel(tex.format) == 4)
                t.pixels = tex.pixels.data();
        }
    }
    // Depth attachment
    if (textures_.Valid(fbo.depthAttachment.id)) {
        auto& dtex = textures_.Get(fbo.depthAttachment.id);
        if (RHIFormatBytesPerPixel(dtex.format) == 4 && RHIFormatIsDepth(dtex.format))
            t.depth = reinterpret_cast<float*>(dtex.pixels.data());
    }
    return t;
}

// -- Helper: 4x4 matrix operations (row-major) ----------------------------

namespace {

inline void Mat4Transpose(const float* in, float* out) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out[r * 4 + c] = in[c * 4 + r];
}

inline void Mat4Mul(const float* A, const float* B, float* out) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) {
            float s = 0;
            for (int k = 0; k < 4; ++k) s += A[r * 4 + k] * B[k * 4 + c];
            out[r * 4 + c] = s;
        }
}

struct Vec4 { float x, y, z, w; };

inline Vec4 Mat4MulVec(const float* M, float x, float y, float z, float w) {
    return {
        M[0]*x + M[1]*y + M[2]*z + M[3]*w,
        M[4]*x + M[5]*y + M[6]*z + M[7]*w,
        M[8]*x + M[9]*y + M[10]*z + M[11]*w,
        M[12]*x + M[13]*y + M[14]*z + M[15]*w
    };
}

inline float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

inline uint8_t F2U8(float v) {
    return static_cast<uint8_t>(Clamp01(v) * 255.0f);
}

} // anon namespace

// -- Triangle rasterization (scene meshes + sky quads) -------------------

void SoftwareRHIDevice::RasterizeTriangles(
    const uint8_t* verts, uint32_t vertexCount, uint32_t firstVertex,
    uint32_t stride, const RHIPipelineDesc& desc, const ColorTarget& target)
{
    if (vertexCount < 3) return;

    // Fetch transform matrices from bound UBO (set=0, binding=0)
    // TransformUBO layout: model[16], view[16], proj[16], cameraPos[4], invVP[16]
    // UBO stores column-major matrices (transposed for std140), so we transpose
    // back to row-major for CPU math.
    float mvp[16];
    float model[16], view[16], proj[16];
    bool hasTransform = false;
    {
        RHIBuffer ubo = boundUniformBuffers_[0][0];
        if (buffers_.Valid(ubo.id)) {
            auto& ubuf = buffers_.Get(ubo.id);
            size_t off = boundUniformOffsets_[0][0];
            // Need at least model+view+proj = 192 bytes
            if (ubuf.data.size() >= off + 192) {
                const float* d = reinterpret_cast<const float*>(ubuf.data.data() + off);
                float colModel[16], colView[16], colProj[16];
                std::memcpy(colModel, d, 64);
                std::memcpy(colView, d + 16, 64);
                std::memcpy(colProj, d + 32, 64);
                Mat4Transpose(colModel, model);
                Mat4Transpose(colView, view);
                Mat4Transpose(colProj, proj);
                float mv[16];
                Mat4Mul(view, model, mv);
                Mat4Mul(proj, mv, mvp);
                hasTransform = true;
            }
        }
    }
    if (!hasTransform) {
        // Identity MVP
        std::memset(mvp, 0, sizeof(mvp));
        mvp[0] = mvp[5] = mvp[10] = mvp[15] = 1.0f;
    }

    // Fetch bound texture (set=2, binding=0) for sampling
    const uint8_t* texPixels = nullptr;
    uint32_t texW = 0, texH = 0;
    {
        RHITexture tex = boundTextures_[2][0];
        if (textures_.Valid(tex.id)) {
            auto& t = textures_.Get(tex.id);
            if (!t.pixels.empty() && RHIFormatBytesPerPixel(t.format) == 4) {
                texPixels = t.pixels.data();
                texW = t.width;
                texH = t.height;
            }
        }
    }

    float vpX = viewport_.x, vpY = viewport_.y;
    float vpW = viewport_.width, vpH = viewport_.height;
    if (vpW <= 0 || vpH <= 0) { vpW = (float)target.width; vpH = (float)target.height; }
    int tw = (int)target.width, th = (int)target.height;

    bool depthTest  = desc.depthStencil.depthTest;
    bool depthWrite = desc.depthStencil.depthWrite;
    bool cullBack   = (desc.rasterizer.cullMode == RHICullMode::Back);

    // Process each triangle
    for (uint32_t i = firstVertex; i + 2 < firstVertex + vertexCount; i += 3) {
        // Read 3 vertices
        struct Vert { float px, py, pz, nx, ny, nz, u, v; };
        Vert v[3];
        for (int k = 0; k < 3; ++k) {
            const uint8_t* vp = verts + (i + k) * stride;
            const float* f = reinterpret_cast<const float*>(vp);
            v[k].px = f[0]; v[k].py = f[1]; v[k].pz = f[2];
            if (stride >= 24) { v[k].nx = f[3]; v[k].ny = f[4]; v[k].nz = f[5]; }
            else { v[k].nx = 0; v[k].ny = 0; v[k].nz = 1; }
            if (stride >= 32) { v[k].u = f[6]; v[k].v = f[7]; }
            else { v[k].u = 0; v[k].v = 0; }
        }

        // Transform to clip space
        Vec4 clip[3];
        for (int k = 0; k < 3; ++k)
            clip[k] = Mat4MulVec(mvp, v[k].px, v[k].py, v[k].pz, 1.0f);

        // Perspective divide -> NDC
        float ndc[3][3]; // x, y, z in NDC
        bool behind = false;
        for (int k = 0; k < 3; ++k) {
            if (clip[k].w <= 0.0001f) { behind = true; break; }
            float invW = 1.0f / clip[k].w;
            ndc[k][0] = clip[k].x * invW;
            ndc[k][1] = clip[k].y * invW;
            ndc[k][2] = clip[k].z * invW;
        }
        if (behind) continue;

        // NDC to screen (viewport transform)
        float sx[3], sy[3], sz[3];
        for (int k = 0; k < 3; ++k) {
            sx[k] = vpX + (ndc[k][0] * 0.5f + 0.5f) * vpW;
            sy[k] = vpY + (1.0f - (ndc[k][1] * 0.5f + 0.5f)) * vpH; // flip Y
            sz[k] = ndc[k][2]; // depth in [0,1] range after projection
        }

        // Backface culling (signed area, CCW = front)
        float area = (sx[1] - sx[0]) * (sy[2] - sy[0]) - (sx[2] - sx[0]) * (sy[1] - sy[0]);
        if (cullBack && area >= 0.0f) continue; // positive area = clockwise = back face

        // Bounding box (clipped to viewport)
        int xMin = std::max(0, (int)std::floor(std::min({sx[0], sx[1], sx[2]})));
        int xMax = std::min(tw - 1, (int)std::ceil(std::max({sx[0], sx[1], sx[2]})));
        int yMin = std::max(0, (int)std::floor(std::min({sy[0], sy[1], sy[2]})));
        int yMax = std::min(th - 1, (int)std::ceil(std::max({sy[0], sy[1], sy[2]})));
        if (xMin > xMax || yMin > yMax) continue;

        // Barycentric setup
        float invArea = 1.0f / area;

        // Edge function deltas for barycentric interpolation
        float dy12 = sy[1] - sy[2], dx21 = sx[2] - sx[1];
        float dy20 = sy[2] - sy[0], dx02 = sx[0] - sx[2];

        // Perspective-correct: inverse w per vertex
        float iw0 = 1.0f / clip[0].w, iw1 = 1.0f / clip[1].w, iw2 = 1.0f / clip[2].w;

        // Pre-compute UV * invW for perspective-correct interpolation
        float u0w = v[0].u * iw0, u1w = v[1].u * iw1, u2w = v[2].u * iw2;
        float v0w = v[0].v * iw0, v1w = v[1].v * iw1, v2w = v[2].v * iw2;

        bool hasTex = (texPixels && texW > 0 && texH > 0);
        constexpr float inv255 = 1.0f / 255.0f;

        // Pre-compute edge function row start values for scanline iteration
        float e0Row = dy12 * ((float)xMin + 0.5f - sx[2]) + dx21 * ((float)yMin + 0.5f - sy[2]);
        float e1Row = dy20 * ((float)xMin + 0.5f - sx[2]) + dx02 * ((float)yMin + 0.5f - sy[2]);

        for (int y = yMin; y <= yMax; ++y) {
            float e0 = e0Row;
            float e1 = e1Row;
            for (int x = xMin; x <= xMax; ++x) {
                float b0 = e0 * invArea;
                float b1 = e1 * invArea;
                float b2 = 1.0f - b0 - b1;

                if (b0 < -1e-4f || b1 < -1e-4f || b2 < -1e-4f) {
                    e0 += dy12; e1 += dy20;
                    continue;
                }

                // Depth interpolation
                float z = b0 * sz[0] + b1 * sz[1] + b2 * sz[2];

                int idx = y * tw + x;
                if (depthTest && target.depth) {
                    if (z >= target.depth[idx]) {
                        e0 += dy12; e1 += dy20;
                        continue;
                    }
                }
                if (depthWrite && target.depth) {
                    target.depth[idx] = z;
                }

                // Perspective-correct UV interpolation
                float iw = b0 * iw0 + b1 * iw1 + b2 * iw2;
                float w = (iw > 1e-8f) ? 1.0f / iw : 1.0f;
                float tu = (b0 * u0w + b1 * u1w + b2 * u2w) * w;
                float tv = (b0 * v0w + b1 * v1w + b2 * v2w) * w;

                // Sample texture or use white
                uint8_t r = 255, g = 255, b_c = 255, a = 255;
                if (hasTex) {
                    // Wrap UVs (fast integer truncation)
                    int itu = static_cast<int>(tu);
                    int itv = static_cast<int>(tv);
                    if (tu < 0.0f) --itu;
                    if (tv < 0.0f) --itv;
                    tu -= static_cast<float>(itu);
                    tv -= static_cast<float>(itv);
                    int tx = static_cast<int>(tu * texW);
                    int ty = static_cast<int>(tv * texH);
                    if (tx >= static_cast<int>(texW)) tx = texW - 1;
                    if (ty >= static_cast<int>(texH)) ty = texH - 1;
                    const uint8_t* tp = texPixels + (ty * texW + tx) * 4;
                    r = tp[0]; g = tp[1]; b_c = tp[2]; a = tp[3];
                }

                // Hemisphere lighting from interpolated normal Y (skip normalize)
                float ny = b0 * v[0].ny + b1 * v[1].ny + b2 * v[2].ny;
                float shade = 0.3f + 0.7f * (ny * 0.5f + 0.5f);

                // Write pixel (RGBA8)
                size_t pi = static_cast<size_t>(idx) * 4;
                if (a < 255) {
                    float af = a * inv255;
                    float inv = 1.0f - af;
                    target.pixels[pi]     = F2U8(r * inv255 * shade * af + target.pixels[pi] * inv255 * inv);
                    target.pixels[pi + 1] = F2U8(g * inv255 * shade * af + target.pixels[pi + 1] * inv255 * inv);
                    target.pixels[pi + 2] = F2U8(b_c * inv255 * shade * af + target.pixels[pi + 2] * inv255 * inv);
                    target.pixels[pi + 3] = F2U8(af + target.pixels[pi + 3] * inv255 * inv);
                } else {
                    // Fast integer path for fully opaque pixels
                    int shade256 = static_cast<int>(shade * 256.0f);
                    target.pixels[pi]     = static_cast<uint8_t>((r * shade256) >> 8);
                    target.pixels[pi + 1] = static_cast<uint8_t>((g * shade256) >> 8);
                    target.pixels[pi + 2] = static_cast<uint8_t>((b_c * shade256) >> 8);
                    target.pixels[pi + 3] = 255;
                }

                e0 += dy12; e1 += dy20;
            }
            e0Row += dx21; e1Row += dx02;
        }
    }
}

// -- UI quad rasterization (2D screen-space triangles) --------------------

void SoftwareRHIDevice::RasterizeUIQuads(
    const uint8_t* verts, uint32_t vertexCount, uint32_t firstVertex,
    uint32_t stride, const RHIPipelineDesc& desc, const ColorTarget& target)
{
    if (vertexCount < 3) return;

    // Fetch bound texture (set=0, binding=0) for font atlas / images
    const uint8_t* texPixels = nullptr;
    uint32_t texW = 0, texH = 0;
    uint8_t texBpp = 0;
    {
        RHITexture tex = boundTextures_[0][0];
        if (textures_.Valid(tex.id)) {
            auto& t = textures_.Get(tex.id);
            if (!t.pixels.empty()) {
                texBpp = RHIFormatBytesPerPixel(t.format);
                if (texBpp == 1 || texBpp == 4) {
                    texPixels = t.pixels.data();
                    texW = t.width;
                    texH = t.height;
                }
            }
        }
    }

    int tw = static_cast<int>(target.width);
    int th = static_cast<int>(target.height);
    bool blendEnabled = desc.blend.enabled;

    // Scissor rect (clamp to framebuffer)
    int scX0 = std::max(0, static_cast<int>(scissor_.x));
    int scY0 = std::max(0, static_cast<int>(scissor_.y));
    int scX1 = std::min(tw, static_cast<int>(scissor_.x + scissor_.width));
    int scY1 = std::min(th, static_cast<int>(scissor_.y + scissor_.height));
    if (scX1 <= scX0 || scY1 <= scY0) return;

    // UIVertex layout (52 bytes):
    //   float x, y       (0)   screen-space position
    //   float u, v       (8)   texture coordinates
    //   uint8_t r,g,b,a  (16)  vertex color
    //   float sdf[4]     (20)  SDF params (halfW, halfH, borderWidth, 0)
    //   float radii[4]   (36)  per-corner radii

    constexpr float inv255 = 1.0f / 255.0f;

    for (uint32_t i = firstVertex; i + 2 < firstVertex + vertexCount; i += 3) {
        struct UIVert { float x, y, u, v; uint8_t r, g, b, a; };
        UIVert v[3];
        float sdf[4] = {0, 0, 0, 0};
        float radii[4] = {0, 0, 0, 0};
        for (int k = 0; k < 3; ++k) {
            const uint8_t* vp = verts + (i + k) * stride;
            const float* f = reinterpret_cast<const float*>(vp);
            v[k].x = f[0]; v[k].y = f[1];
            v[k].u = f[2]; v[k].v = f[3];
            v[k].r = vp[16]; v[k].g = vp[17]; v[k].b = vp[18]; v[k].a = vp[19];
        }
        // SDF params and radii are constant across the quad - read from first vertex
        {
            const float* f0 = reinterpret_cast<const float*>(verts + i * stride);
            sdf[0] = f0[5]; sdf[1] = f0[6]; sdf[2] = f0[7]; sdf[3] = f0[8];
            radii[0] = f0[9]; radii[1] = f0[10]; radii[2] = f0[11]; radii[3] = f0[12];
        }
        bool hasSDF = (sdf[0] > 0.0f || sdf[1] > 0.0f);
        bool hasRadius = (radii[0] > 0.0f || radii[1] > 0.0f ||
                          radii[2] > 0.0f || radii[3] > 0.0f);

        // Bounding box clipped to viewport AND scissor
        float minX = std::min({v[0].x, v[1].x, v[2].x});
        float maxX = std::max({v[0].x, v[1].x, v[2].x});
        float minY = std::min({v[0].y, v[1].y, v[2].y});
        float maxY = std::max({v[0].y, v[1].y, v[2].y});

        int xMin = std::max(scX0, static_cast<int>(std::floor(minX)));
        int xMax = std::min(scX1 - 1, static_cast<int>(std::ceil(maxX)));
        int yMin = std::max(scY0, static_cast<int>(std::floor(minY)));
        int yMax = std::min(scY1 - 1, static_cast<int>(std::ceil(maxY)));
        if (xMin > xMax || yMin > yMax) continue;

        // Barycentric setup
        float area = (v[1].x - v[0].x) * (v[2].y - v[0].y)
                   - (v[2].x - v[0].x) * (v[1].y - v[0].y);
        if (std::fabs(area) < 1e-6f) continue;
        float invArea = 1.0f / area;

        float dy12 = v[1].y - v[2].y, dx21 = v[2].x - v[1].x;
        float dy20 = v[2].y - v[0].y, dx02 = v[0].x - v[2].x;

        // Incremental edge function setup
        float e0Row = dy12 * ((float)xMin + 0.5f - v[2].x) + dx21 * ((float)yMin + 0.5f - v[2].y);
        float e1Row = dy20 * ((float)xMin + 0.5f - v[2].x) + dx02 * ((float)yMin + 0.5f - v[2].y);

        for (int y = yMin; y <= yMax; ++y) {
            float e0 = e0Row;
            float e1 = e1Row;
            for (int x = xMin; x <= xMax; ++x) {
                float b0 = e0 * invArea;
                float b1 = e1 * invArea;
                float b2 = 1.0f - b0 - b1;

                if (b0 < -1e-4f || b1 < -1e-4f || b2 < -1e-4f) {
                    e0 += dy12; e1 += dy20;
                    continue;
                }

                // Interpolate UV
                float tu = b0 * v[0].u + b1 * v[1].u + b2 * v[2].u;
                float tv = b0 * v[0].v + b1 * v[1].v + b2 * v[2].v;

                // SDF rounded-rect evaluation
                float sdfAlpha = 1.0f;
                if (hasSDF && hasRadius) {
                    float halfW = sdf[0], halfH = sdf[1];
                    float lx = (tu - 0.5f) * 2.0f * halfW;
                    float ly = (tv - 0.5f) * 2.0f * halfH;
                    float cr = (lx > 0.0f)
                        ? ((ly > 0.0f) ? radii[2] : radii[1])
                        : ((ly > 0.0f) ? radii[3] : radii[0]);
                    float qx = std::fabs(lx) - halfW + cr;
                    float qy = std::fabs(ly) - halfH + cr;
                    float qxc = qx > 0.0f ? qx : 0.0f;
                    float qyc = qy > 0.0f ? qy : 0.0f;
                    float d = std::sqrt(qxc * qxc + qyc * qyc)
                            + std::min(std::max(qx, qy), 0.0f) - cr;
                    if (d > 0.5f) { e0 += dy12; e1 += dy20; continue; }
                    sdfAlpha = Clamp01(0.5f - d);
                }

                // Interpolate vertex color
                float cr_c = b0 * v[0].r + b1 * v[1].r + b2 * v[2].r;
                float cg   = b0 * v[0].g + b1 * v[1].g + b2 * v[2].g;
                float cb   = b0 * v[0].b + b1 * v[1].b + b2 * v[2].b;
                float ca   = b0 * v[0].a + b1 * v[1].a + b2 * v[2].a;

                // Sample texture if available (font atlas)
                float texR = 1.0f, texG = 1.0f, texB = 1.0f, texA = 1.0f;
                if (texPixels && texW > 0 && texH > 0) {
                    int sx = static_cast<int>(Clamp01(tu) * (texW - 1));
                    int sy = static_cast<int>(Clamp01(tv) * (texH - 1));
                    if (texBpp == 1) {
                        uint8_t val = texPixels[static_cast<size_t>(sy) * texW + sx];
                        texA = val * inv255;
                    } else {
                        size_t si = (static_cast<size_t>(sy) * texW + sx) * 4;
                        texR = texPixels[si] * inv255;
                        texG = texPixels[si + 1] * inv255;
                        texB = texPixels[si + 2] * inv255;
                        texA = texPixels[si + 3] * inv255;
                    }
                }

                // Modulate vertex color * texture * SDF alpha
                float fr = (cr_c * inv255) * texR;
                float fg = (cg * inv255) * texG;
                float fb = (cb * inv255) * texB;
                float fa = (ca * inv255) * texA * sdfAlpha;

                size_t pi = (static_cast<size_t>(y) * tw + x) * 4;

                if (blendEnabled && fa < 1.0f - 1e-4f) {
                    float inv = 1.0f - fa;
                    target.pixels[pi]     = F2U8(fr * fa + target.pixels[pi] * inv255 * inv);
                    target.pixels[pi + 1] = F2U8(fg * fa + target.pixels[pi + 1] * inv255 * inv);
                    target.pixels[pi + 2] = F2U8(fb * fa + target.pixels[pi + 2] * inv255 * inv);
                    target.pixels[pi + 3] = F2U8(fa + target.pixels[pi + 3] * inv255 * inv);
                } else {
                    target.pixels[pi]     = F2U8(fr);
                    target.pixels[pi + 1] = F2U8(fg);
                    target.pixels[pi + 2] = F2U8(fb);
                    target.pixels[pi + 3] = F2U8(fa);
                }

                e0 += dy12; e1 += dy20;
            }
            e0Row += dx21; e1Row += dx02;
        }
    }
}

// -- Line rasterization (debug lines) ------------------------------------

void SoftwareRHIDevice::RasterizeLines(
    const uint8_t* verts, uint32_t vertexCount, uint32_t firstVertex,
    uint32_t stride, const RHIPipelineDesc& desc, const ColorTarget& target)
{
    if (vertexCount < 2) return;

    // Fetch MVP from transform UBO (column-major -> row-major transpose)
    float mvp[16];
    bool hasTransform = false;
    {
        RHIBuffer ubo = boundUniformBuffers_[0][0];
        if (buffers_.Valid(ubo.id)) {
            auto& ubuf = buffers_.Get(ubo.id);
            size_t off = boundUniformOffsets_[0][0];
            if (ubuf.data.size() >= off + 192) {
                const float* d = reinterpret_cast<const float*>(ubuf.data.data() + off);
                float colModel[16], colView[16], colProj[16];
                float model[16], view[16], proj[16], mv[16];
                std::memcpy(colModel, d, 64);
                std::memcpy(colView, d + 16, 64);
                std::memcpy(colProj, d + 32, 64);
                Mat4Transpose(colModel, model);
                Mat4Transpose(colView, view);
                Mat4Transpose(colProj, proj);
                Mat4Mul(view, model, mv);
                Mat4Mul(proj, mv, mvp);
                hasTransform = true;
            }
        }
    }
    if (!hasTransform) {
        std::memset(mvp, 0, sizeof(mvp));
        mvp[0] = mvp[5] = mvp[10] = mvp[15] = 1.0f;
    }

    float vpX = viewport_.x, vpY = viewport_.y;
    float vpW = viewport_.width, vpH = viewport_.height;
    if (vpW <= 0 || vpH <= 0) { vpW = (float)target.width; vpH = (float)target.height; }
    int tw = (int)target.width, th = (int)target.height;

    bool depthTest  = desc.depthStencil.depthTest;
    bool depthWrite = desc.depthStencil.depthWrite;

    // DebugLineVertex: pos(3f) + color(4f) = 28 bytes
    for (uint32_t i = firstVertex; i + 1 < firstVertex + vertexCount; i += 2) {
        const float* f0 = reinterpret_cast<const float*>(verts + i * stride);
        const float* f1 = reinterpret_cast<const float*>(verts + (i + 1) * stride);

        // Transform endpoints
        Vec4 c0 = Mat4MulVec(mvp, f0[0], f0[1], f0[2], 1.0f);
        Vec4 c1 = Mat4MulVec(mvp, f1[0], f1[1], f1[2], 1.0f);
        if (c0.w <= 0.0001f || c1.w <= 0.0001f) continue;

        float s0x = vpX + (c0.x / c0.w * 0.5f + 0.5f) * vpW;
        float s0y = vpY + (1.0f - (c0.y / c0.w * 0.5f + 0.5f)) * vpH;
        float s0z = c0.z / c0.w;
        float s1x = vpX + (c1.x / c1.w * 0.5f + 0.5f) * vpW;
        float s1y = vpY + (1.0f - (c1.y / c1.w * 0.5f + 0.5f)) * vpH;
        float s1z = c1.z / c1.w;

        // Line color (from first vertex, RGBA float at offset 12)
        uint8_t lr = F2U8(f0[3]), lg = F2U8(f0[4]), lb = F2U8(f0[5]), la = F2U8(f0[6]);

        // Bresenham line rasterization
        int x0 = (int)std::round(s0x), y0 = (int)std::round(s0y);
        int x1 = (int)std::round(s1x), y1 = (int)std::round(s1y);
        int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
        int err = dx - dy;
        int steps = dx + dy;
        if (steps == 0) steps = 1;

        int cx = x0, cy = y0;
        for (int s = 0; s <= steps; ++s) {
            if (cx >= 0 && cx < tw && cy >= 0 && cy < th) {
                int idx = cy * tw + cx;
                float t = (steps > 0) ? (float)s / (float)steps : 0.0f;
                float z = s0z + t * (s1z - s0z);

                bool pass = true;
                if (depthTest && target.depth && z >= target.depth[idx]) pass = false;

                if (pass) {
                    if (depthWrite && target.depth) target.depth[idx] = z;
                    size_t pi = static_cast<size_t>(idx) * 4;
                    target.pixels[pi]     = lr;
                    target.pixels[pi + 1] = lg;
                    target.pixels[pi + 2] = lb;
                    target.pixels[pi + 3] = la;
                }
            }
            if (cx == x1 && cy == y1) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; cx += sx; }
            if (e2 <  dx) { err += dx; cy += sy; }
        }
    }
}

// -- Fullscreen quad blit (copies offscreen texture to target) -----------

void SoftwareRHIDevice::BlitFullscreenQuad(
    const uint8_t* /*verts*/, uint32_t /*vertexCount*/, uint32_t /*firstVertex*/,
    uint32_t /*stride*/, const ColorTarget& target)
{
    // Search sets 0, 1, 2 for the first valid RGBA texture
    RHITexture srcTex{};
    for (uint32_t s = 0; s < kMaxSets; ++s) {
        RHITexture t = boundTextures_[s][0];
        if (textures_.Valid(t.id)) {
            auto& tex = textures_.Get(t.id);
            if (!tex.pixels.empty() && RHIFormatBytesPerPixel(tex.format) == 4) {
                srcTex = t;
                break;
            }
        }
    }
    if (!textures_.Valid(srcTex.id)) return;

    auto& src = textures_.Get(srcTex.id);
    if (src.pixels.empty() || RHIFormatBytesPerPixel(src.format) != 4) return;

    uint32_t srcW = src.width, srcH = src.height;
    uint32_t dstW = target.width, dstH = target.height;

    // Fast path: same size, opaque copy
    if (srcW == dstW && srcH == dstH) {
        std::memcpy(target.pixels, src.pixels.data(), srcW * srcH * 4);
        return;
    }

    // Nearest-neighbor scale with alpha blending
    for (uint32_t y = 0; y < dstH; ++y) {
        uint32_t sy = (y * srcH) / dstH;
        if (sy >= srcH) sy = srcH - 1;
        for (uint32_t x = 0; x < dstW; ++x) {
            uint32_t sx = (x * srcW) / dstW;
            if (sx >= srcW) sx = srcW - 1;

            size_t si = (static_cast<size_t>(sy) * srcW + sx) * 4;
            size_t di = (static_cast<size_t>(y) * dstW + x) * 4;

            uint8_t a = src.pixels[si + 3];
            if (a == 255) {
                target.pixels[di]     = src.pixels[si];
                target.pixels[di + 1] = src.pixels[si + 1];
                target.pixels[di + 2] = src.pixels[si + 2];
                target.pixels[di + 3] = 255;
            } else if (a > 0) {
                float af = a / 255.0f, inv = 1.0f - af;
                target.pixels[di]     = F2U8(src.pixels[si] / 255.0f * af + target.pixels[di] / 255.0f * inv);
                target.pixels[di + 1] = F2U8(src.pixels[si + 1] / 255.0f * af + target.pixels[di + 1] / 255.0f * inv);
                target.pixels[di + 2] = F2U8(src.pixels[si + 2] / 255.0f * af + target.pixels[di + 2] / 255.0f * inv);
                target.pixels[di + 3] = F2U8(af + target.pixels[di + 3] / 255.0f * inv);
            }
        }
    }
}

} // namespace koilo::rhi
