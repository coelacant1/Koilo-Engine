// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file vulkan_rhi_device.hpp
 * @brief Vulkan implementation of IRHIDevice.
 *
 * Maps RHI opaque handles to Vulkan objects via internal slot arrays.
 * Receives VkDevice/VkQueue from the existing VulkanBackend display
 * layer - does NOT create its own Vulkan instance or device.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */
#pragma once
#include "../rhi_device.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <mutex>

// Forward-declare VulkanBackend so callers don't need the full header.
namespace koilo { class VulkanBackend; }

namespace koilo::rhi {

/// Vulkan implementation of IRHIDevice.
///
/// Slot arrays map uint32_t handle IDs to native Vulkan objects.  Slot 0
/// is reserved (null sentinel).  Freed slots are recycled via a free list.
class VulkanRHIDevice final : public IRHIDevice {
public:
    /// Construct with a pointer to the display backend that owns the device.
    explicit VulkanRHIDevice(VulkanBackend* display);
    ~VulkanRHIDevice() override;

    // -- Lifecycle ---------------------------------------------------
    bool Initialize() override;
    void Shutdown()   override;
    const char* GetName() const override { return "Vulkan RHI"; }

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
                       size_t size, size_t offset) override;
    void  UpdateTexture(RHITexture handle, const void* data,
                        size_t dataSize,
                        uint32_t width, uint32_t height) override;
    void* MapBuffer(RHIBuffer handle) override;
    void  UnmapBuffer(RHIBuffer handle) override;

    // -- Command recording -------------------------------------------
    void BeginFrame() override;
    void BeginRenderPass(RHIRenderPass pass, RHIFramebuffer framebuffer,
                          const RHIClearValue& clear) override;
    void EndRenderPass() override;
    void BindPipeline(RHIPipeline pipeline) override;
    void BindVertexBuffer(RHIBuffer buffer,
                           uint32_t binding, uint64_t offset) override;
    void BindIndexBuffer(RHIBuffer buffer,
                          bool is32Bit, uint64_t offset) override;
    void BindUniformBuffer(RHIBuffer buffer, uint32_t set,
                            uint32_t binding,
                            size_t offset, size_t range) override;
    void BindTexture(RHITexture texture,
                      uint32_t set, uint32_t binding) override;
    void SetViewport(const RHIViewport& vp) override;
    void SetScissor(const RHIScissor& sc) override;
    void PushConstants(RHIShaderStage stages, const void* data,
                        uint32_t size, uint32_t offset) override;
    void Draw(uint32_t vertexCount, uint32_t instanceCount,
               uint32_t firstVertex, uint32_t firstInstance) override;
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                      uint32_t firstIndex, int32_t vertexOffset,
                      uint32_t firstInstance) override;
    void EndFrame() override;
    void Present() override;

    // -- Swapchain ---------------------------------------------------
    void GetSwapchainSize(uint32_t& outWidth,
                           uint32_t& outHeight) const override;
    void OnResize(uint32_t width, uint32_t height) override;

private:
    // -- Slot array for handle - Vulkan object mapping ---------------
    template<typename T>
    struct SlotArray {
        std::vector<T>        slots;   // index 0 is unused (null sentinel)
        std::vector<uint32_t> freeList;

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

        T& Get(uint32_t id) { return slots[id]; }
        const T& Get(uint32_t id) const { return slots[id]; }
        bool Valid(uint32_t id) const { return id > 0 && id < slots.size(); }
    };

    // -- Native slot entries -----------------------------------------

    struct BufferSlot {
        VkBuffer       buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize   size   = 0;
        bool           hostVisible = false;
        void*          mapped = nullptr;
    };

    struct TextureSlot {
        VkImage        image   = VK_NULL_HANDLE;
        VkDeviceMemory memory  = VK_NULL_HANDLE;
        VkImageView    view    = VK_NULL_HANDLE;
        VkSampler      sampler = VK_NULL_HANDLE;
        uint32_t       width   = 0;
        uint32_t       height  = 0;
        VkFormat       format  = VK_FORMAT_UNDEFINED;
    };

    struct ShaderSlot {
        VkShaderModule module = VK_NULL_HANDLE;
        RHIShaderStage stage  = RHIShaderStage::Vertex;
    };

    struct PipelineSlot {
        VkPipeline       pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout   = VK_NULL_HANDLE;
        bool             ownsLayout = false;
    };

    struct RenderPassSlot {
        VkRenderPass pass = VK_NULL_HANDLE;
    };

    struct FramebufferSlot {
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        uint32_t      width       = 0;
        uint32_t      height      = 0;
    };

    // -- Vulkan helpers ----------------------------------------------

    uint32_t FindMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags props) const;

    bool CreateVkBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags memProps,
                        VkBuffer& outBuffer, VkDeviceMemory& outMemory);

    bool CreateVkImage(uint32_t w, uint32_t h, VkFormat fmt,
                       VkImageUsageFlags usage,
                       VkImage& outImage, VkDeviceMemory& outMemory);

    VkImageView CreateVkImageView(VkImage image, VkFormat fmt,
                                   VkImageAspectFlags aspect);

    void TransitionImageLayout(VkCommandBuffer cmd, VkImage image,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout,
                               VkImageAspectFlags aspect);

    VkCommandBuffer BeginOneShot();
    void            EndOneShot(VkCommandBuffer cmd);

    static VkFormat       ToVkFormat(RHIFormat fmt);
    static VkBufferUsageFlags ToVkBufferUsage(RHIBufferUsage usage);

    // -- State -------------------------------------------------------

    VulkanBackend*   display_ = nullptr;
    VkDevice         device_  = VK_NULL_HANDLE;
    VkPhysicalDevice physDevice_ = VK_NULL_HANDLE;
    VkQueue          graphicsQueue_ = VK_NULL_HANDLE;
    uint32_t         graphicsFamily_ = 0;
    bool             initialized_ = false;

    VkCommandPool    cmdPool_ = VK_NULL_HANDLE;
    VkCommandBuffer  cmdBuffer_ = VK_NULL_HANDLE;
    VkFence          frameFence_ = VK_NULL_HANDLE;
    bool             frameActive_ = false;

    // Slot arrays
    SlotArray<BufferSlot>      buffers_;
    SlotArray<TextureSlot>     textures_;
    SlotArray<ShaderSlot>      shaders_;
    SlotArray<PipelineSlot>    pipelines_;
    SlotArray<RenderPassSlot>  renderPasses_;
    SlotArray<FramebufferSlot> framebuffers_;

    RHIFeature supportedFeatures_ = static_cast<RHIFeature>(0);
    RHILimits  limits_{};

    // Descriptor management (simple single-set layout for now)
    VkDescriptorPool      descriptorPool_  = VK_NULL_HANDLE;
    VkDescriptorSetLayout uniformLayout_   = VK_NULL_HANDLE;
    VkDescriptorSetLayout textureLayout_   = VK_NULL_HANDLE;
    VkPipelineLayout      defaultPipelineLayout_ = VK_NULL_HANDLE;
};

} // namespace koilo::rhi
