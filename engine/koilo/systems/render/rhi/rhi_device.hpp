// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file rhi_device.hpp
 * @brief Render Hardware Interface - pure virtual device interface.
 *
 * IRHIDevice is the sole abstraction that backends implement.  It covers
 * resource lifecycle, command recording, and presentation.  All methods
 * use the opaque handles and descriptors from rhi_types.hpp.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */
#pragma once
#include "rhi_types.hpp"
#include "rhi_caps.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace koilo::rhi {

/// Pure virtual GPU device interface.
///
/// Implementations (Vulkan, OpenGL, etc.) map the opaque handles returned
/// by Create* methods to native API objects via internal slot arrays.
/// The null handle (id == 0) is never returned on success.
class IRHIDevice {
public:
    virtual ~IRHIDevice() = default;

    // -- Lifecycle ---------------------------------------------------
    virtual bool Initialize() = 0;
    virtual void Shutdown()   = 0;
    virtual const char* GetName() const = 0;

    // -- Capability queries ------------------------------------------
    virtual bool      SupportsFeature(RHIFeature feature) const = 0;
    virtual RHILimits GetLimits() const = 0;

    // -- Resource creation / destruction -----------------------------

    virtual RHIBuffer     CreateBuffer(const RHIBufferDesc& desc) = 0;
    virtual void          DestroyBuffer(RHIBuffer handle) = 0;

    virtual RHITexture    CreateTexture(const RHITextureDesc& desc) = 0;
    virtual void          DestroyTexture(RHITexture handle) = 0;

    /// Create a shader module from SPIR-V or GLSL bytecode.
    virtual RHIShader     CreateShader(RHIShaderStage stage,
                                       const void* code,
                                       size_t codeSize) = 0;
    virtual void          DestroyShader(RHIShader handle) = 0;

    virtual RHIPipeline   CreatePipeline(const RHIPipelineDesc& desc) = 0;
    virtual void          DestroyPipeline(RHIPipeline handle) = 0;

    virtual RHIRenderPass CreateRenderPass(const RHIRenderPassDesc& desc) = 0;
    virtual void          DestroyRenderPass(RHIRenderPass handle) = 0;

    virtual RHIFramebuffer CreateFramebuffer(RHIRenderPass pass,
                                              const RHITexture* colorAttachments,
                                              uint32_t colorCount,
                                              RHITexture depthAttachment,
                                              uint32_t width, uint32_t height) = 0;
    virtual void           DestroyFramebuffer(RHIFramebuffer handle) = 0;

    // -- Data transfer -----------------------------------------------

    /// Upload data to a buffer.  offset + size must fit within buffer.
    virtual void UpdateBuffer(RHIBuffer handle,
                              const void* data, size_t size,
                              size_t offset = 0) = 0;

    /// Upload pixel data to a texture (mip 0, layer 0).
    virtual void UpdateTexture(RHITexture handle,
                               const void* data, size_t dataSize,
                               uint32_t width, uint32_t height) = 0;

    /// Map a host-visible buffer.  Returns nullptr on failure.
    virtual void* MapBuffer(RHIBuffer handle) = 0;
    virtual void  UnmapBuffer(RHIBuffer handle) = 0;

    // -- Command recording -------------------------------------------
    // Commands are recorded between BeginFrame/EndFrame.  The backend
    // manages command buffer allocation internally.

    /// Begin a new frame.  Acquires the next swapchain image (if applicable).
    virtual void BeginFrame() = 0;

    /// Begin a render pass into the given framebuffer with clear values.
    virtual void BeginRenderPass(RHIRenderPass pass,
                                  RHIFramebuffer framebuffer,
                                  const RHIClearValue& clear) = 0;
    virtual void EndRenderPass() = 0;

    /// Bind a graphics pipeline for subsequent draw calls.
    virtual void BindPipeline(RHIPipeline pipeline) = 0;

    /// Bind a vertex buffer at the given binding slot.
    virtual void BindVertexBuffer(RHIBuffer buffer,
                                   uint32_t binding = 0,
                                   uint64_t offset = 0) = 0;

    /// Bind an index buffer (uint16 or uint32).
    virtual void BindIndexBuffer(RHIBuffer buffer,
                                  bool is32Bit = true,
                                  uint64_t offset = 0) = 0;

    /// Bind a uniform buffer to a descriptor set binding.
    virtual void BindUniformBuffer(RHIBuffer buffer,
                                    uint32_t set,
                                    uint32_t binding,
                                    size_t offset = 0,
                                    size_t range = 0) = 0;

    /// Bind a texture + sampler to a descriptor set binding.
    virtual void BindTexture(RHITexture texture,
                              uint32_t set,
                              uint32_t binding) = 0;

    virtual void SetViewport(const RHIViewport& vp) = 0;
    virtual void SetScissor(const RHIScissor& sc) = 0;

    /// Push constants (Vulkan-style; GL backends may ignore or emulate).
    virtual void PushConstants(RHIShaderStage stages,
                                const void* data,
                                uint32_t size,
                                uint32_t offset = 0) = 0;

    virtual void Draw(uint32_t vertexCount,
                       uint32_t instanceCount = 1,
                       uint32_t firstVertex = 0,
                       uint32_t firstInstance = 0) = 0;

    virtual void DrawIndexed(uint32_t indexCount,
                              uint32_t instanceCount = 1,
                              uint32_t firstIndex = 0,
                              int32_t  vertexOffset = 0,
                              uint32_t firstInstance = 0) = 0;

    /// End the current frame.  Submits recorded commands.
    virtual void EndFrame() = 0;

    /// Present the rendered frame to the display.
    virtual void Present() = 0;

    // -- Swapchain / surface -----------------------------------------

    /// Get current swapchain dimensions.
    virtual void GetSwapchainSize(uint32_t& outWidth,
                                   uint32_t& outHeight) const = 0;

    /// Notify the device that the window was resized.
    virtual void OnResize(uint32_t width, uint32_t height) = 0;
};

} // namespace koilo::rhi
