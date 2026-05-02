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

    /// Returns true if the backend supports partial texture uploads via
    /// UpdateTextureRegion.  When false, callers should perform a full
    /// UpdateTexture and ignore region helpers.  Used by the canvas
    /// overlay path to opt into dirty-rect uploads only when the active
    /// backend can preserve untouched pixels.
    virtual bool SupportsTextureSubUpdate() const { return false; }

    /// Upload `data` (tightly packed, srcRowPitch = w * bytesPerPixel)
    /// into the rectangle [x, y, x+w, y+h) of texture `handle`,
    /// preserving pixels outside the region.  Default impl falls back
    /// to a full UpdateTexture treating `data` as a w*h image (which
    /// is incorrect for partial uploads - callers must gate on
    /// SupportsTextureSubUpdate()).
    virtual void UpdateTextureRegion(RHITexture handle,
                                     const void* data, size_t dataSize,
                                     uint32_t x, uint32_t y,
                                     uint32_t w, uint32_t h) {
        (void)x; (void)y;
        UpdateTexture(handle, data, dataSize, w, h);
    }

    /// Stage a full-texture upload as part of the active frame's command
    /// buffer.  Callers MUST invoke this between BeginFrame() and the first
    /// BeginRenderPass() / BeginSwapchainRenderPass() (vkCmdCopyBufferToImage
    /// is not permitted inside a render pass).
    ///
    /// On backends that implement this (e.g. Vulkan), the upload is recorded
    /// into the frame command buffer using a per-frame-in-flight persistent
    /// staging buffer - no per-call vkAllocateMemory / vkQueueSubmit /
    /// vkQueueWaitIdle.  The default impl falls back to UpdateTexture which
    /// on most backends is a synchronous one-shot submit.
    ///
    /// Returns true on success.  On failure (e.g. no active frame, allocation
    /// failure), callers should NOT advance dirty-tracking state because the
    /// GPU contents may be stale.
    virtual bool StageTextureFull(RHITexture handle,
                                  const void* data, size_t dataSize,
                                  uint32_t w, uint32_t h) {
        UpdateTexture(handle, data, dataSize, w, h);
        return true;
    }

    /// Stage a region upload as part of the active frame's command buffer.
    /// See StageTextureFull for semantics and call-site requirements.
    virtual bool StageTextureRegion(RHITexture handle,
                                    const void* data, size_t dataSize,
                                    uint32_t x, uint32_t y,
                                    uint32_t w, uint32_t h) {
        UpdateTextureRegion(handle, data, dataSize, x, y, w, h);
        return true;
    }

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

    /// Begin a render pass that targets the swapchain (screen).
    /// The device manages the swapchain framebuffer internally.
    /// Default implementation does nothing (override in GPU backends).
    virtual void BeginSwapchainRenderPass(const RHIClearValue& clear) {
        (void)clear;
    }

    /// Get the render pass handle used for swapchain rendering.
    /// Pipelines that render to the screen (blit, overlay) must be
    /// created with this render pass for compatibility.
    /// Returns an invalid handle if the device doesn't support it.
    virtual RHIRenderPass GetSwapchainRenderPass() const { return {}; }

    /// Get current swapchain dimensions.
    virtual void GetSwapchainSize(uint32_t& outWidth,
                                   uint32_t& outHeight) const = 0;

    /// Notify the device that the window was resized.
    virtual void OnResize(uint32_t width, uint32_t height) = 0;

    // -- Timestamp queries -------------------------------------------
    // Optional GPU timing support.  Backends that support TimestampQueries
    // override these; the defaults are safe no-ops.

    /// Reset the internal timestamp query pool for a new frame.
    /// @param maxQueries  maximum number of WriteTimestamp() calls this frame.
    virtual void ResetTimestamps(uint32_t maxQueries) { (void)maxQueries; }

    /// Record a GPU timestamp at the current point in the command stream.
    /// @param index  slot index (0 .. maxQueries-1).
    virtual void WriteTimestamp(uint32_t index) { (void)index; }

    /// Read back timestamp results from the previous frame.
    /// @param out    array of at least @p count uint64_t values.
    /// @param count  number of timestamps to read.
    /// @return true if results are available.
    virtual bool ReadTimestamps(uint64_t* out, uint32_t count) {
        (void)out; (void)count; return false;
    }

    /// Nanoseconds per timestamp tick.  Multiply raw deltas by this value.
    virtual double GetTimestampPeriod() const { return 0.0; }

    // -- Synchronization ------------------------------------------------

    /// Block until all previously submitted GPU work has completed.
    /// Use sparingly - causes a full GPU stall.  Intended for resource
    /// teardown (e.g. hot-reload pipeline invalidation).
    virtual void WaitIdle() {}

    // -- Software pixel readback ----------------------------------------

    /// Return a pointer to the CPU-side swapchain pixel buffer (RGBA8).
    /// Non-null only for software RHI backends; GPU backends return nullptr.
    /// Used by the host to upload the rendered frame to the display.
    virtual const uint8_t* GetSwapchainPixels() const { return nullptr; }
};

} // namespace koilo::rhi
