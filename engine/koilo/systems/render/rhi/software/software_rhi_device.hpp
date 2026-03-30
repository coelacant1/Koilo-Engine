// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file software_rhi_device.hpp
 * @brief Software (CPU) implementation of IRHIDevice.
 *
 * Maps RHI opaque handles to CPU-side data via internal slot arrays,
 * following the same pattern as the OpenGL and Vulkan backends.
 * All resources live in system memory.  Draw() calls are recorded
 * as no-ops in this skeleton phase -- actual CPU rasterization will
 * be added in a subsequent phase.
 *
 * This allows the entire FrameComposer -> RenderPipeline -> IRHIDevice
 * path to execute identically regardless of backend, with software
 * rendering treated as just another device layer.
 *
 * @date 03/29/2026
 * @author Coela Can't
 */
#pragma once
#include "../rhi_device.hpp"

#include <vector>
#include <cstdint>
#include <cstring>

namespace koilo::rhi {

class SoftwareRHIDevice final : public IRHIDevice {
public:
    SoftwareRHIDevice() = default;
    ~SoftwareRHIDevice() override;

    // -- Lifecycle ---------------------------------------------------
    bool        Initialize() override;
    void        Shutdown() override;
    const char* GetName() const override { return "Software RHI"; }

    // -- Capabilities ------------------------------------------------
    bool      SupportsFeature(RHIFeature feature) const override;
    RHILimits GetLimits() const override;

    // -- Resource creation / destruction -----------------------------
    RHIBuffer     CreateBuffer(const RHIBufferDesc& desc) override;
    void          DestroyBuffer(RHIBuffer handle) override;

    RHITexture    CreateTexture(const RHITextureDesc& desc) override;
    void          DestroyTexture(RHITexture handle) override;

    RHIShader     CreateShader(RHIShaderStage stage,
                               const void* code, size_t codeSize) override;
    void          DestroyShader(RHIShader handle) override;

    RHIPipeline   CreatePipeline(const RHIPipelineDesc& desc) override;
    void          DestroyPipeline(RHIPipeline handle) override;

    RHIRenderPass CreateRenderPass(const RHIRenderPassDesc& desc) override;
    void          DestroyRenderPass(RHIRenderPass handle) override;

    RHIFramebuffer CreateFramebuffer(RHIRenderPass pass,
                                      const RHITexture* colorAttachments,
                                      uint32_t colorCount,
                                      RHITexture depthAttachment,
                                      uint32_t width, uint32_t height) override;
    void           DestroyFramebuffer(RHIFramebuffer handle) override;

    // -- Data transfer -----------------------------------------------
    void  UpdateBuffer(RHIBuffer handle, const void* data,
                       size_t size, size_t offset = 0) override;
    void  UpdateTexture(RHITexture handle, const void* data,
                        size_t dataSize,
                        uint32_t width, uint32_t height) override;
    void* MapBuffer(RHIBuffer handle) override;
    void  UnmapBuffer(RHIBuffer handle) override;

    // -- Command recording -------------------------------------------
    void BeginFrame() override;
    void BeginRenderPass(RHIRenderPass pass,
                         RHIFramebuffer framebuffer,
                         const RHIClearValue& clear) override;
    void EndRenderPass() override;

    void BindPipeline(RHIPipeline pipeline) override;
    void BindVertexBuffer(RHIBuffer buffer,
                          uint32_t binding = 0,
                          uint64_t offset = 0) override;
    void BindIndexBuffer(RHIBuffer buffer,
                         bool is32Bit = true,
                         uint64_t offset = 0) override;
    void BindUniformBuffer(RHIBuffer buffer,
                           uint32_t set, uint32_t binding,
                           size_t offset = 0,
                           size_t range = 0) override;
    void BindTexture(RHITexture texture,
                     uint32_t set, uint32_t binding) override;

    void SetViewport(const RHIViewport& vp) override;
    void SetScissor(const RHIScissor& sc) override;
    void PushConstants(RHIShaderStage stages,
                       const void* data,
                       uint32_t size,
                       uint32_t offset = 0) override;

    void Draw(uint32_t vertexCount,
              uint32_t instanceCount = 1,
              uint32_t firstVertex = 0,
              uint32_t firstInstance = 0) override;
    void DrawIndexed(uint32_t indexCount,
                     uint32_t instanceCount = 1,
                     uint32_t firstIndex = 0,
                     int32_t  vertexOffset = 0,
                     uint32_t firstInstance = 0) override;

    void EndFrame() override;
    void Present() override;

    // -- Swapchain / surface -----------------------------------------
    void BeginSwapchainRenderPass(const RHIClearValue& clear) override;
    RHIRenderPass GetSwapchainRenderPass() const override;
    void GetSwapchainSize(uint32_t& outWidth, uint32_t& outHeight) const override;
    void OnResize(uint32_t width, uint32_t height) override;

    // -- Software-specific API ---------------------------------------

    /// Access the final composited pixel buffer (RGBA8, width * height * 4 bytes).
    /// Returns nullptr if no swapchain buffer has been allocated.
    const uint8_t* GetSwapchainPixels() const override;

    /// Access the swapchain pixel buffer dimensions.
    uint32_t GetSwapchainWidth() const { return swapchainWidth_; }
    uint32_t GetSwapchainHeight() const { return swapchainHeight_; }

private:
    // -- Slot array (same pattern as OpenGL/Vulkan backends) ---------
    template <typename T>
    struct SlotArray {
        std::vector<T>        slots;
        std::vector<uint32_t> freeList;

        SlotArray() { slots.push_back(T{}); } // slot 0 = null sentinel

        uint32_t Alloc(const T& item) {
            uint32_t id;
            if (!freeList.empty()) {
                id = freeList.back();
                freeList.pop_back();
                slots[id] = item;
            } else {
                id = static_cast<uint32_t>(slots.size());
                slots.push_back(item);
            }
            return id;
        }

        void Free(uint32_t id) {
            if (id == 0 || id >= slots.size()) return;
            slots[id] = T{};
            freeList.push_back(id);
        }

        T&       Get(uint32_t id)       { return slots[id]; }
        const T& Get(uint32_t id) const { return slots[id]; }
        bool     Valid(uint32_t id) const {
            return id > 0 && id < slots.size();
        }

        void Clear() {
            slots.clear();
            slots.push_back(T{});
            freeList.clear();
        }
    };

    // -- CPU-side resource slots -------------------------------------

    struct BufferSlot {
        std::vector<uint8_t> data;
        RHIBufferUsage       usage = RHIBufferUsage::Vertex;
        bool                 hostVisible = false;
    };

    struct TextureSlot {
        std::vector<uint8_t> pixels;
        uint32_t  width    = 0;
        uint32_t  height   = 0;
        RHIFormat format   = RHIFormat::RGBA8_Unorm;
        RHISamplerFilter filter = RHISamplerFilter::Linear;
    };

    struct ShaderSlot {
        RHIShaderStage stage = RHIShaderStage::Vertex;
        // Bytecode stored for future introspection (not executed)
        std::vector<uint8_t> code;
    };

    struct PipelineSlot {
        RHIPipelineDesc desc = {};
    };

    struct RenderPassSlot {
        RHIRenderPassDesc desc = {};
    };

    struct FramebufferSlot {
        RHIRenderPass             renderPass = {};
        std::vector<RHITexture>   colorAttachments;
        RHITexture                depthAttachment = {};
        uint32_t                  width  = 0;
        uint32_t                  height = 0;
    };

    // -- Slot arrays -------------------------------------------------
    SlotArray<BufferSlot>      buffers_;
    SlotArray<TextureSlot>     textures_;
    SlotArray<ShaderSlot>      shaders_;
    SlotArray<PipelineSlot>    pipelines_;
    SlotArray<RenderPassSlot>  renderPasses_;
    SlotArray<FramebufferSlot> framebuffers_;

    // -- Bound state (current pipeline / resource bindings) ----------
    static constexpr uint32_t kMaxSets     = 4;
    static constexpr uint32_t kMaxBindings = 16;

    RHIPipeline boundPipeline_ = {};
    RHIBuffer   boundVertexBuffers_[kMaxBindings] = {};
    uint64_t    boundVertexOffsets_[kMaxBindings]  = {};
    RHIBuffer   boundIndexBuffer_ = {};
    bool        boundIndexIs32Bit_ = true;
    RHIBuffer   boundUniformBuffers_[kMaxSets][kMaxBindings] = {};
    size_t      boundUniformOffsets_[kMaxSets][kMaxBindings]  = {};
    size_t      boundUniformRanges_[kMaxSets][kMaxBindings]   = {};
    RHITexture  boundTextures_[kMaxSets][kMaxBindings] = {};

    RHIViewport viewport_ = {};
    RHIScissor  scissor_  = {};

    // Active render pass state
    RHIRenderPass  activeRenderPass_  = {};
    RHIFramebuffer activeFramebuffer_ = {};
    bool           renderingToSwapchain_ = false;

    // -- Swapchain emulation -----------------------------------------
    uint32_t swapchainWidth_  = 0;
    uint32_t swapchainHeight_ = 0;
    std::vector<uint8_t> swapchainPixels_;   // RGBA8 framebuffer
    std::vector<float>   swapchainDepth_;    // depth buffer for swapchain
    RHIRenderPass swapchainRenderPass_ = {};

    bool initialized_ = false;

    // -- Rasterization helpers (SW-2) --------------------------------

    /// Get the active color buffer (framebuffer attachment or swapchain).
    /// Returns {pointer, width, height} or {nullptr, 0, 0} if none.
    struct ColorTarget {
        uint8_t* pixels = nullptr;
        float*   depth  = nullptr;
        uint32_t width  = 0;
        uint32_t height = 0;
    };
    ColorTarget GetActiveColorTarget();

    void RasterizeTriangles(const uint8_t* verts, uint32_t vertexCount,
                            uint32_t firstVertex, uint32_t stride,
                            const RHIPipelineDesc& desc,
                            const ColorTarget& target);
    void RasterizeUIQuads(const uint8_t* verts, uint32_t vertexCount,
                          uint32_t firstVertex, uint32_t stride,
                          const RHIPipelineDesc& desc,
                          const ColorTarget& target);
    void RasterizeLines(const uint8_t* verts, uint32_t vertexCount,
                        uint32_t firstVertex, uint32_t stride,
                        const RHIPipelineDesc& desc,
                        const ColorTarget& target);
    void BlitFullscreenQuad(const uint8_t* verts, uint32_t vertexCount,
                            uint32_t firstVertex, uint32_t stride,
                            const ColorTarget& target);
};

} // namespace koilo::rhi
