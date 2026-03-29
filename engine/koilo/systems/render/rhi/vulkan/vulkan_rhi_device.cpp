// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file vulkan_rhi_device.cpp
 * @brief Vulkan implementation of IRHIDevice - resource management,
 *        command recording, and presentation via slot-array mapping.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */
#include "vulkan_rhi_device.hpp"
#include <koilo/systems/display/backends/gpu/vulkanbackend.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <cstring>
#include <algorithm>

namespace koilo::rhi {

// -- Format conversion tables ----------------------------------------

VkFormat VulkanRHIDevice::ToVkFormat(RHIFormat fmt) {
    switch (fmt) {
        case RHIFormat::R8_Unorm:           return VK_FORMAT_R8_UNORM;
        case RHIFormat::RG8_Unorm:          return VK_FORMAT_R8G8_UNORM;
        case RHIFormat::RGB8_Unorm:         return VK_FORMAT_R8G8B8_UNORM;
        case RHIFormat::RGBA8_Unorm:        return VK_FORMAT_R8G8B8A8_UNORM;
        case RHIFormat::BGRA8_Unorm:        return VK_FORMAT_B8G8R8A8_UNORM;
        case RHIFormat::RGBA8_SRGB:         return VK_FORMAT_R8G8B8A8_SRGB;
        case RHIFormat::BGRA8_SRGB:         return VK_FORMAT_B8G8R8A8_SRGB;
        case RHIFormat::RG16F:              return VK_FORMAT_R16G16_SFLOAT;
        case RHIFormat::RGBA16F:            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case RHIFormat::RGBA32F:            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case RHIFormat::RGB32F:             return VK_FORMAT_R32G32B32_SFLOAT;
        case RHIFormat::RG32F:              return VK_FORMAT_R32G32_SFLOAT;
        case RHIFormat::R32F:               return VK_FORMAT_R32_SFLOAT;
        case RHIFormat::D16_Unorm:          return VK_FORMAT_D16_UNORM;
        case RHIFormat::D24_Unorm_S8_Uint:  return VK_FORMAT_D24_UNORM_S8_UINT;
        case RHIFormat::D32F:               return VK_FORMAT_D32_SFLOAT;
        case RHIFormat::D32F_S8_Uint:       return VK_FORMAT_D32_SFLOAT_S8_UINT;
        default:                            return VK_FORMAT_UNDEFINED;
    }
}

VkBufferUsageFlags VulkanRHIDevice::ToVkBufferUsage(RHIBufferUsage usage) {
    VkBufferUsageFlags flags = 0;
    if (HasFlag(usage, RHIBufferUsage::Vertex))   flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (HasFlag(usage, RHIBufferUsage::Index))     flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (HasFlag(usage, RHIBufferUsage::Uniform))   flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (HasFlag(usage, RHIBufferUsage::Storage))   flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (HasFlag(usage, RHIBufferUsage::Staging))   flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (HasFlag(usage, RHIBufferUsage::Indirect))  flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    // All non-staging buffers can be transfer destinations (for upload)
    if (!HasFlag(usage, RHIBufferUsage::Staging))
        flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    return flags;
}

// -- Constructor / Destructor ----------------------------------------

VulkanRHIDevice::VulkanRHIDevice(VulkanBackend* display)
    : display_(display)
{
    // Reserve slot 0 as null sentinel for all slot arrays
    buffers_.slots.emplace_back();
    textures_.slots.emplace_back();
    shaders_.slots.emplace_back();
    pipelines_.slots.emplace_back();
    renderPasses_.slots.emplace_back();
    framebuffers_.slots.emplace_back();
}

VulkanRHIDevice::~VulkanRHIDevice() {
    if (initialized_) Shutdown();
}

// -- Lifecycle -------------------------------------------------------

bool VulkanRHIDevice::Initialize() {
    if (initialized_) return true;
    if (!display_ || !display_->IsInitialized()) {
        KL_ERR("RHI", "VulkanBackend display not initialized");
        return false;
    }

    device_         = display_->GetDevice();
    physDevice_     = display_->GetPhysicalDevice();
    graphicsQueue_  = display_->GetGraphicsQueue();
    graphicsFamily_ = display_->GetGraphicsFamily();

    // Create command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamily_;
    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &cmdPool_) != VK_SUCCESS) {
        KL_ERR("RHI", "Failed to create command pool");
        return false;
    }

    // Allocate primary command buffer
    VkCommandBufferAllocateInfo cmdAlloc{};
    cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAlloc.commandPool = cmdPool_;
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(device_, &cmdAlloc, &cmdBuffer_) != VK_SUCCESS) {
        KL_ERR("RHI", "Failed to allocate command buffer");
        return false;
    }

    // Create frame fence
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateFence(device_, &fenceInfo, nullptr, &frameFence_) != VK_SUCCESS) {
        KL_ERR("RHI", "Failed to create frame fence");
        return false;
    }

    // Create per-frame descriptor pools (one per frame in flight).
    // Each frame only resets its own pool after its fence is waited on,
    // avoiding the race where one frame's pool is reset while the other
    // frame's command buffer still references its descriptor sets.
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         512},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,  64},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 512},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         128},
    };
    VkDescriptorPoolCreateInfo dpInfo{};
    dpInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    dpInfo.maxSets = 1024;
    dpInfo.poolSizeCount = 4;
    dpInfo.pPoolSizes = poolSizes;
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (vkCreateDescriptorPool(device_, &dpInfo, nullptr,
                                   &descriptorPools_[i]) != VK_SUCCESS) {
            KL_ERR("RHI", "Failed to create descriptor pool %u", i);
            return false;
        }
    }

    // -- Scene pipeline descriptor set layouts --------------------------
    // Matches KSL SPIR-V binding conventions:
    //   Set 0: scene data (transform UBO, light SSBO, scene UBO, audio SSBO)
    //   Set 1: material UBO
    //   Set 2: texture samplers (up to 8)

    // Set 0: Scene data - 4 bindings
    {
        VkDescriptorSetLayoutBinding bindings[4] = {};
        // Binding 0: Transform UBO (vertex + fragment)
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        // Binding 1: Light buffer SSBO (fragment)
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        // Binding 2: Scene globals UBO (fragment)
        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        // Binding 3: Audio samples SSBO (fragment)
        bindings[3].binding = 3;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[3].descriptorCount = 1;
        bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 4;
        layoutInfo.pBindings = bindings;
        if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &sceneSetLayout_) != VK_SUCCESS) {
            KL_ERR("RHI", "Failed to create scene descriptor set layout");
            return false;
        }
    }
    // Set 1: Material UBO
    {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;
        if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &materialSetLayout_) != VK_SUCCESS) {
            KL_ERR("RHI", "Failed to create material descriptor set layout");
            return false;
        }
    }
    // Set 2: Texture samplers (up to 8)
    {
        std::array<VkDescriptorSetLayoutBinding, 8> bindings{};
        for (uint32_t i = 0; i < 8; ++i) {
            bindings[i].binding = i;
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 8;
        layoutInfo.pBindings = bindings.data();
        if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &textureSetLayout_) != VK_SUCCESS) {
            KL_ERR("RHI", "Failed to create texture descriptor set layout");
            return false;
        }
    }
    // Scene pipeline layout: 3 sets + push constants
    {
        VkDescriptorSetLayout layouts[] = {sceneSetLayout_, materialSetLayout_, textureSetLayout_};

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushRange.offset = 0;
        pushRange.size = 128;

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 3;
        layoutInfo.pSetLayouts = layouts;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;

        if (vkCreatePipelineLayout(device_, &layoutInfo, nullptr,
                                    &scenePipelineLayout_) != VK_SUCCESS) {
            KL_ERR("RHI", "Failed to create scene pipeline layout");
            return false;
        }
    }

    // -- Blit pipeline descriptor set layouts ---------------------------
    // Set 0: one combined image sampler (the offscreen color texture)
    {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;
        if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &blitSamplerLayout_) != VK_SUCCESS) {
            KL_ERR("RHI", "Failed to create blit sampler layout");
            return false;
        }
    }
    // Set 1: optional UBO (overlay alpha, etc.)
    {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;
        if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &blitUBOLayout_) != VK_SUCCESS) {
            KL_ERR("RHI", "Failed to create blit UBO layout");
            return false;
        }
    }
    // Blit pipeline layout: 2 sets + push constants (used by UI shaders)
    {
        VkDescriptorSetLayout layouts[] = {blitSamplerLayout_, blitUBOLayout_};

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushRange.offset = 0;
        pushRange.size = 128;

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 2;
        layoutInfo.pSetLayouts = layouts;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;

        if (vkCreatePipelineLayout(device_, &layoutInfo, nullptr,
                                    &blitPipelineLayout_) != VK_SUCCESS) {
            KL_ERR("RHI", "Failed to create blit pipeline layout");
            return false;
        }
    }

    // Query device limits
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physDevice_, &props);
    limits_.maxTextureSize        = props.limits.maxImageDimension2D;
    limits_.maxTexture3DSize      = props.limits.maxImageDimension3D;
    limits_.maxCubeMapSize        = props.limits.maxImageDimensionCube;
    limits_.maxFramebufferWidth   = props.limits.maxFramebufferWidth;
    limits_.maxFramebufferHeight  = props.limits.maxFramebufferHeight;
    limits_.maxColorAttachments   = props.limits.maxColorAttachments;
    limits_.maxVertexAttributes   = props.limits.maxVertexInputAttributes;
    limits_.maxVertexBindings     = props.limits.maxVertexInputBindings;
    limits_.maxUniformBufferSize  = props.limits.maxUniformBufferRange;
    limits_.maxStorageBufferSize  = props.limits.maxStorageBufferRange;
    limits_.maxPushConstantSize   = props.limits.maxPushConstantsSize;
    limits_.maxBoundDescriptorSets = props.limits.maxBoundDescriptorSets;
    limits_.maxSamplers           = props.limits.maxDescriptorSetSamplers;
    limits_.maxAnisotropy         = props.limits.maxSamplerAnisotropy;

    // -- Dynamic uniform ring buffer ----------------------------------
    // A large HOST_COHERENT buffer shared by all per-draw uniform writes.
    // Each UpdateBuffer for a uniform buffer writes to the NEXT slice,
    // so GPU sees per-draw data instead of the last-written overwrite.
    uniformMinAlign_ = static_cast<uint32_t>(
        props.limits.minUniformBufferOffsetAlignment);
    if (uniformMinAlign_ == 0) uniformMinAlign_ = 256;
    uniformRingSize_ = 4u * 1024u * 1024u; // 4 MB
    if (!CreateVkBuffer(uniformRingSize_,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        uniformRingBuffer_, uniformRingMemory_)) {
        KL_ERR("RHI", "Failed to create uniform ring buffer");
        return false;
    }
    vkMapMemory(device_, uniformRingMemory_, 0, uniformRingSize_,
                0, &uniformRingMapped_);
    uniformRingOffset_ = 0;

    // Query features
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(physDevice_, &features);

    uint16_t featureBits = 0;
    if (props.limits.timestampComputeAndGraphics)
        featureBits |= static_cast<uint16_t>(RHIFeature::TimestampQueries);
    featureBits |= static_cast<uint16_t>(RHIFeature::PushConstants);
    featureBits |= static_cast<uint16_t>(RHIFeature::StorageBuffers);
    if (features.multiDrawIndirect)
        featureBits |= static_cast<uint16_t>(RHIFeature::MultiDrawIndirect);
    if (features.geometryShader)
        featureBits |= static_cast<uint16_t>(RHIFeature::GeometryShaders);
    if (features.tessellationShader)
        featureBits |= static_cast<uint16_t>(RHIFeature::TessellationShaders);
    if (features.depthClamp)
        featureBits |= static_cast<uint16_t>(RHIFeature::DepthClamp);
    supportedFeatures_ = static_cast<RHIFeature>(featureBits);

    initialized_ = true;
    KL_LOG("RHI", "Vulkan RHI initialized");
    return true;
}

void VulkanRHIDevice::Shutdown() {
    if (!initialized_) return;
    vkDeviceWaitIdle(device_);

    // Flush all deferred texture deletions across all frame slots
    for (uint32_t f = 0; f < kMaxFramesInFlight; ++f) {
        for (auto& s : pendingTextureDeletes_[f]) {
            if (s.sampler != VK_NULL_HANDLE) vkDestroySampler(device_, s.sampler, nullptr);
            if (s.view != VK_NULL_HANDLE)    vkDestroyImageView(device_, s.view, nullptr);
            if (s.image != VK_NULL_HANDLE)   vkDestroyImage(device_, s.image, nullptr);
            if (s.memory != VK_NULL_HANDLE)  vkFreeMemory(device_, s.memory, nullptr);
        }
        pendingTextureDeletes_[f].clear();
    }

    // Flush all deferred buffer deletions across all frame slots
    for (uint32_t f = 0; f < kMaxFramesInFlight; ++f) {
        for (auto& s : pendingBufferDeletes_[f]) {
            if (s.mapped)  vkUnmapMemory(device_, s.memory);
            if (s.buffer != VK_NULL_HANDLE) vkDestroyBuffer(device_, s.buffer, nullptr);
            if (s.memory != VK_NULL_HANDLE) vkFreeMemory(device_, s.memory, nullptr);
        }
        pendingBufferDeletes_[f].clear();
    }

    // Destroy all remaining resources
    for (uint32_t i = 1; i < static_cast<uint32_t>(framebuffers_.slots.size()); ++i) {
        auto& s = framebuffers_.slots[i];
        if (s.framebuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(device_, s.framebuffer, nullptr);
    }
    for (uint32_t i = 1; i < static_cast<uint32_t>(pipelines_.slots.size()); ++i) {
        auto& s = pipelines_.slots[i];
        if (s.pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(device_, s.pipeline, nullptr);
        if (s.ownsLayout && s.layout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(device_, s.layout, nullptr);
    }
    for (uint32_t i = 1; i < static_cast<uint32_t>(renderPasses_.slots.size()); ++i) {
        // Skip the swapchain pass - its VkRenderPass is owned by the display
        if (swapchainPassHandle_.IsValid() && i == swapchainPassHandle_.id) continue;
        auto& s = renderPasses_.slots[i];
        if (s.pass != VK_NULL_HANDLE)
            vkDestroyRenderPass(device_, s.pass, nullptr);
    }
    for (uint32_t i = 1; i < static_cast<uint32_t>(shaders_.slots.size()); ++i) {
        auto& s = shaders_.slots[i];
        if (s.module != VK_NULL_HANDLE)
            vkDestroyShaderModule(device_, s.module, nullptr);
    }
    for (uint32_t i = 1; i < static_cast<uint32_t>(textures_.slots.size()); ++i) {
        auto& s = textures_.slots[i];
        if (s.sampler != VK_NULL_HANDLE) vkDestroySampler(device_, s.sampler, nullptr);
        if (s.view != VK_NULL_HANDLE)    vkDestroyImageView(device_, s.view, nullptr);
        if (s.image != VK_NULL_HANDLE)   vkDestroyImage(device_, s.image, nullptr);
        if (s.memory != VK_NULL_HANDLE)  vkFreeMemory(device_, s.memory, nullptr);
    }
    for (uint32_t i = 1; i < static_cast<uint32_t>(buffers_.slots.size()); ++i) {
        auto& s = buffers_.slots[i];
        if (s.mapped) vkUnmapMemory(device_, s.memory);
        if (s.buffer != VK_NULL_HANDLE) vkDestroyBuffer(device_, s.buffer, nullptr);
        if (s.memory != VK_NULL_HANDLE) vkFreeMemory(device_, s.memory, nullptr);
    }

    // Destroy the uniform ring buffer
    if (uniformRingMapped_) {
        vkUnmapMemory(device_, uniformRingMemory_);
        uniformRingMapped_ = nullptr;
    }
    if (uniformRingBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, uniformRingBuffer_, nullptr);
        uniformRingBuffer_ = VK_NULL_HANDLE;
    }
    if (uniformRingMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, uniformRingMemory_, nullptr);
        uniformRingMemory_ = VK_NULL_HANDLE;
    }

    // Destroy pipeline layouts
    if (scenePipelineLayout_ != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device_, scenePipelineLayout_, nullptr);
    if (blitPipelineLayout_ != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device_, blitPipelineLayout_, nullptr);
    // Destroy descriptor set layouts
    if (sceneSetLayout_ != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device_, sceneSetLayout_, nullptr);
    if (materialSetLayout_ != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device_, materialSetLayout_, nullptr);
    if (textureSetLayout_ != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device_, textureSetLayout_, nullptr);
    if (blitSamplerLayout_ != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device_, blitSamplerLayout_, nullptr);
    if (blitUBOLayout_ != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device_, blitUBOLayout_, nullptr);
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (descriptorPools_[i] != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(device_, descriptorPools_[i], nullptr);
    }
    if (frameFence_ != VK_NULL_HANDLE)
        vkDestroyFence(device_, frameFence_, nullptr);
    if (cmdPool_ != VK_NULL_HANDLE)
        vkDestroyCommandPool(device_, cmdPool_, nullptr);

    initialized_ = false;
    KL_LOG("RHI", "Vulkan RHI shut down");
}

// -- Capability queries ----------------------------------------------

bool VulkanRHIDevice::SupportsFeature(RHIFeature feature) const {
    return HasFeature(supportedFeatures_, feature);
}

RHILimits VulkanRHIDevice::GetLimits() const {
    return limits_;
}

// -- Vulkan helpers --------------------------------------------------

uint32_t VulkanRHIDevice::FindMemoryType(uint32_t typeFilter,
                                          VkMemoryPropertyFlags props) const {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physDevice_, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    return UINT32_MAX;
}

bool VulkanRHIDevice::CreateVkBuffer(VkDeviceSize size,
                                      VkBufferUsageFlags usage,
                                      VkMemoryPropertyFlags memProps,
                                      VkBuffer& outBuffer,
                                      VkDeviceMemory& outMemory) {
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &info, nullptr, &outBuffer) != VK_SUCCESS)
        return false;

    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(device_, outBuffer, &reqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = reqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(reqs.memoryTypeBits, memProps);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        vkDestroyBuffer(device_, outBuffer, nullptr);
        outBuffer = VK_NULL_HANDLE;
        return false;
    }

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &outMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device_, outBuffer, nullptr);
        outBuffer = VK_NULL_HANDLE;
        return false;
    }

    vkBindBufferMemory(device_, outBuffer, outMemory, 0);
    return true;
}

bool VulkanRHIDevice::CreateVkImage(uint32_t w, uint32_t h, VkFormat fmt,
                                     VkImageUsageFlags usage,
                                     VkImage& outImage,
                                     VkDeviceMemory& outMemory) {
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = fmt;
    info.extent = {w, h, 1};
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device_, &info, nullptr, &outImage) != VK_SUCCESS)
        return false;

    VkMemoryRequirements reqs;
    vkGetImageMemoryRequirements(device_, outImage, &reqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = reqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(reqs.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        vkDestroyImage(device_, outImage, nullptr);
        outImage = VK_NULL_HANDLE;
        return false;
    }

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &outMemory) != VK_SUCCESS) {
        vkDestroyImage(device_, outImage, nullptr);
        outImage = VK_NULL_HANDLE;
        return false;
    }

    vkBindImageMemory(device_, outImage, outMemory, 0);
    return true;
}

VkImageView VulkanRHIDevice::CreateVkImageView(VkImage image, VkFormat fmt,
                                                VkImageAspectFlags aspect) {
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = fmt;
    info.subresourceRange.aspectMask = aspect;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;

    VkImageView view;
    if (vkCreateImageView(device_, &info, nullptr, &view) != VK_SUCCESS)
        return VK_NULL_HANDLE;
    return view;
}

void VulkanRHIDevice::TransitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                             VkImageLayout oldLayout,
                                             VkImageLayout newLayout,
                                             VkImageAspectFlags aspect) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);
}

VkCommandBuffer VulkanRHIDevice::BeginOneShot() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = cmdPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device_, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    return cmd;
}

void VulkanRHIDevice::EndOneShot(VkCommandBuffer cmd) {
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue_);
    vkFreeCommandBuffers(device_, cmdPool_, 1, &cmd);
}

// -- Resource creation -----------------------------------------------

RHIBuffer VulkanRHIDevice::CreateBuffer(const RHIBufferDesc& desc) {
    VkBufferUsageFlags vkUsage = ToVkBufferUsage(desc.usage);
    VkMemoryPropertyFlags memProps = desc.hostVisible
        ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    BufferSlot slot{};
    slot.size = desc.size;
    slot.hostVisible = desc.hostVisible;
    slot.isUniform = HasFlag(desc.usage, RHIBufferUsage::Uniform);

    if (!CreateVkBuffer(desc.size, vkUsage, memProps, slot.buffer, slot.memory)) {
        KL_ERR("RHI", "Failed to create buffer (%s)", desc.debugName ? desc.debugName : "unnamed");
        return {};
    }

    return {buffers_.Alloc(slot)};
}

void VulkanRHIDevice::DestroyBuffer(RHIBuffer handle) {
    if (!handle.IsValid() || !buffers_.Valid(handle.id)) return;
    auto& s = buffers_.Get(handle.id);

    if (frameActive_) {
        // Defer destruction - command buffer may still reference this buffer.
        // Queue to the current frame slot's deferred list; it will be flushed
        // when this slot is reused (after its fence has been waited on).
        pendingBufferDeletes_[currentPoolIndex_].push_back(s);
    } else {
        if (s.mapped) { vkUnmapMemory(device_, s.memory); s.mapped = nullptr; }
        if (s.buffer != VK_NULL_HANDLE) vkDestroyBuffer(device_, s.buffer, nullptr);
        if (s.memory != VK_NULL_HANDLE) vkFreeMemory(device_, s.memory, nullptr);
    }

    // Clear handles so Shutdown() slot iteration won't double-destroy.
    s.buffer = VK_NULL_HANDLE;
    s.memory = VK_NULL_HANDLE;
    s.mapped = nullptr;
    buffers_.Free(handle.id);
}

RHITexture VulkanRHIDevice::CreateTexture(const RHITextureDesc& desc) {
    VkFormat vkFmt = ToVkFormat(desc.format);
    VkImageUsageFlags usage = 0;
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;

    if (HasFlag(desc.usage, RHITextureUsage::Sampled))
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (HasFlag(desc.usage, RHITextureUsage::RenderTarget))
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (HasFlag(desc.usage, RHITextureUsage::DepthStencil)) {
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (RHIFormatHasStencil(desc.format))
            aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    if (HasFlag(desc.usage, RHITextureUsage::TransferSrc))
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (HasFlag(desc.usage, RHITextureUsage::TransferDst))
        usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (HasFlag(desc.usage, RHITextureUsage::Storage))
        usage |= VK_IMAGE_USAGE_STORAGE_BIT;

    TextureSlot slot{};
    slot.width  = desc.width;
    slot.height = desc.height;
    slot.format = vkFmt;

    if (!CreateVkImage(desc.width, desc.height, vkFmt, usage,
                       slot.image, slot.memory)) {
        KL_ERR("RHI", "Failed to create texture (%s)",
               desc.debugName ? desc.debugName : "unnamed");
        return {};
    }

    slot.view = CreateVkImageView(slot.image, vkFmt, aspect);

    // Create default sampler
    VkFilter vkFilter = (desc.filter == RHISamplerFilter::Nearest)
                        ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = vkFilter;
    samplerInfo.minFilter = vkFilter;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.maxLod = static_cast<float>(desc.mipLevels);
    vkCreateSampler(device_, &samplerInfo, nullptr, &slot.sampler);

    return {textures_.Alloc(slot)};
}

void VulkanRHIDevice::DestroyTexture(RHITexture handle) {
    if (!handle.IsValid() || !textures_.Valid(handle.id)) return;
    auto& s = textures_.Get(handle.id);

    if (frameActive_) {
        // Defer destruction - command buffer may still reference this texture.
        // Queue to the current frame slot's deferred list; it will be flushed
        // when this slot is reused (after its fence has been waited on).
        pendingTextureDeletes_[currentPoolIndex_].push_back(s);
    } else {
        if (s.sampler != VK_NULL_HANDLE) vkDestroySampler(device_, s.sampler, nullptr);
        if (s.view != VK_NULL_HANDLE)    vkDestroyImageView(device_, s.view, nullptr);
        if (s.image != VK_NULL_HANDLE)   vkDestroyImage(device_, s.image, nullptr);
        if (s.memory != VK_NULL_HANDLE)  vkFreeMemory(device_, s.memory, nullptr);
    }

    // Clear handles so Shutdown() slot iteration won't double-destroy.
    s.sampler = VK_NULL_HANDLE;
    s.view    = VK_NULL_HANDLE;
    s.image   = VK_NULL_HANDLE;
    s.memory  = VK_NULL_HANDLE;
    textures_.Free(handle.id);
}

RHIShader VulkanRHIDevice::CreateShader(RHIShaderStage stage,
                                         const void* code, size_t codeSize) {
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = codeSize;
    info.pCode = reinterpret_cast<const uint32_t*>(code);

    ShaderSlot slot{};
    slot.stage = stage;
    if (vkCreateShaderModule(device_, &info, nullptr, &slot.module) != VK_SUCCESS) {
        KL_ERR("RHI", "Failed to create shader module");
        return {};
    }
    return {shaders_.Alloc(slot)};
}

void VulkanRHIDevice::DestroyShader(RHIShader handle) {
    if (!handle.IsValid() || !shaders_.Valid(handle.id)) return;
    auto& s = shaders_.Get(handle.id);
    if (s.module != VK_NULL_HANDLE) vkDestroyShaderModule(device_, s.module, nullptr);
    s.module = VK_NULL_HANDLE;
    shaders_.Free(handle.id);
}

RHIPipeline VulkanRHIDevice::CreatePipeline(const RHIPipelineDesc& desc) {
    // Shader stages
    VkPipelineShaderStageCreateInfo stages[2]{};
    uint32_t stageCount = 0;

    if (desc.vertexShader.IsValid() && shaders_.Valid(desc.vertexShader.id)) {
        auto& vs = shaders_.Get(desc.vertexShader.id);
        stages[stageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[stageCount].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[stageCount].module = vs.module;
        stages[stageCount].pName = "main";
        ++stageCount;
    }
    if (desc.fragmentShader.IsValid() && shaders_.Valid(desc.fragmentShader.id)) {
        auto& fs = shaders_.Get(desc.fragmentShader.id);
        stages[stageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[stageCount].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[stageCount].module = fs.module;
        stages[stageCount].pName = "main";
        ++stageCount;
    }

    // Vertex input
    VkVertexInputAttributeDescription vkAttrs[RHIPipelineDesc::kMaxVertexAttrs]{};
    for (uint32_t i = 0; i < desc.vertexAttrCount; ++i) {
        vkAttrs[i].location = desc.vertexAttrs[i].location;
        vkAttrs[i].binding  = 0;
        vkAttrs[i].format   = ToVkFormat(desc.vertexAttrs[i].format);
        vkAttrs[i].offset   = desc.vertexAttrs[i].offset;
    }

    VkVertexInputBindingDescription vertexBinding{};
    vertexBinding.binding   = 0;
    vertexBinding.stride    = desc.vertexStride;
    vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount   = desc.vertexStride > 0 ? 1u : 0u;
    vertexInput.pVertexBindingDescriptions      = &vertexBinding;
    vertexInput.vertexAttributeDescriptionCount = desc.vertexAttrCount;
    vertexInput.pVertexAttributeDescriptions    = vkAttrs;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    switch (desc.topology) {
        case RHITopology::TriangleStrip: inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
        case RHITopology::LineList:      inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;      break;
        case RHITopology::LineStrip:     inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;     break;
        case RHITopology::PointList:     inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;     break;
        default:                         inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  break;
    }

    // Dynamic viewport/scissor
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = desc.rasterizer.depthClamp ? VK_TRUE : VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;
    switch (desc.rasterizer.polygonMode) {
        case RHIPolygonMode::Line:  rasterizer.polygonMode = VK_POLYGON_MODE_LINE;  break;
        case RHIPolygonMode::Point: rasterizer.polygonMode = VK_POLYGON_MODE_POINT; break;
        default:                    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  break;
    }
    switch (desc.rasterizer.cullMode) {
        case RHICullMode::None:         rasterizer.cullMode = VK_CULL_MODE_NONE;           break;
        case RHICullMode::Front:        rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;      break;
        case RHICullMode::FrontAndBack: rasterizer.cullMode = VK_CULL_MODE_FRONT_AND_BACK; break;
        default:                        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;       break;
    }
    rasterizer.frontFace = (desc.rasterizer.frontFace == RHIFrontFace::Clockwise)
        ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;

    // Multisampling (none)
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth/stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable  = desc.depthStencil.depthTest  ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = desc.depthStencil.depthWrite ? VK_TRUE : VK_FALSE;
    depthStencil.stencilTestEnable = desc.depthStencil.stencilTest ? VK_TRUE : VK_FALSE;
    switch (desc.depthStencil.depthOp) {
        case RHICompareOp::Never:        depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;            break;
        case RHICompareOp::Equal:        depthStencil.depthCompareOp = VK_COMPARE_OP_EQUAL;            break;
        case RHICompareOp::LessEqual:    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;    break;
        case RHICompareOp::Greater:      depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;          break;
        case RHICompareOp::NotEqual:     depthStencil.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL;        break;
        case RHICompareOp::GreaterEqual: depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; break;
        case RHICompareOp::Always:       depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;           break;
        default:                         depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;             break;
    }

    // Color blend
    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                               | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttach.blendEnable = desc.blend.enabled ? VK_TRUE : VK_FALSE;
    if (desc.blend.enabled) {
        auto toVkBlendFactor = [](RHIBlendFactor f) -> VkBlendFactor {
            switch (f) {
                case RHIBlendFactor::Zero:               return VK_BLEND_FACTOR_ZERO;
                case RHIBlendFactor::One:                return VK_BLEND_FACTOR_ONE;
                case RHIBlendFactor::SrcAlpha:           return VK_BLEND_FACTOR_SRC_ALPHA;
                case RHIBlendFactor::OneMinusSrcAlpha:   return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                case RHIBlendFactor::DstAlpha:           return VK_BLEND_FACTOR_DST_ALPHA;
                case RHIBlendFactor::OneMinusDstAlpha:   return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                case RHIBlendFactor::SrcColor:           return VK_BLEND_FACTOR_SRC_COLOR;
                case RHIBlendFactor::OneMinusSrcColor:   return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                case RHIBlendFactor::DstColor:           return VK_BLEND_FACTOR_DST_COLOR;
                case RHIBlendFactor::OneMinusDstColor:   return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                default: return VK_BLEND_FACTOR_ONE;
            }
        };
        auto toVkBlendOp = [](RHIBlendOp o) -> VkBlendOp {
            switch (o) {
                case RHIBlendOp::Subtract:        return VK_BLEND_OP_SUBTRACT;
                case RHIBlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
                case RHIBlendOp::Min:             return VK_BLEND_OP_MIN;
                case RHIBlendOp::Max:             return VK_BLEND_OP_MAX;
                default:                          return VK_BLEND_OP_ADD;
            }
        };
        blendAttach.srcColorBlendFactor = toVkBlendFactor(desc.blend.srcColor);
        blendAttach.dstColorBlendFactor = toVkBlendFactor(desc.blend.dstColor);
        blendAttach.colorBlendOp        = toVkBlendOp(desc.blend.colorOp);
        blendAttach.srcAlphaBlendFactor = toVkBlendFactor(desc.blend.srcAlpha);
        blendAttach.dstAlphaBlendFactor = toVkBlendFactor(desc.blend.dstAlpha);
        blendAttach.alphaBlendOp        = toVkBlendOp(desc.blend.alphaOp);
    }

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &blendAttach;

    // Render pass
    VkRenderPass vkPass = VK_NULL_HANDLE;
    if (desc.renderPass.IsValid() && renderPasses_.Valid(desc.renderPass.id))
        vkPass = renderPasses_.Get(desc.renderPass.id).pass;

    // Select pipeline layout based on hint
    VkPipelineLayout selectedLayout = scenePipelineLayout_;
    if (desc.layoutHint == RHIPipelineDesc::LayoutHint::Blit)
        selectedLayout = blitPipelineLayout_;

    // Create pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = stageCount;
    pipelineInfo.pStages             = stages;
    pipelineInfo.pVertexInputState   = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.pColorBlendState    = &colorBlend;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = selectedLayout;
    pipelineInfo.renderPass          = vkPass;
    pipelineInfo.subpass             = 0;

    PipelineSlot slot{};
    slot.layout = selectedLayout;
    slot.ownsLayout = false;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo,
                                   nullptr, &slot.pipeline) != VK_SUCCESS) {
        KL_ERR("RHI", "Failed to create pipeline (%s)",
               desc.debugName ? desc.debugName : "unnamed");
        return {};
    }

    uint32_t pipeId = pipelines_.Alloc(slot);
    return {pipeId};
}

void VulkanRHIDevice::DestroyPipeline(RHIPipeline handle) {
    if (!handle.IsValid() || !pipelines_.Valid(handle.id)) return;
    auto& s = pipelines_.Get(handle.id);
    if (s.pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device_, s.pipeline, nullptr);
    if (s.ownsLayout && s.layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device_, s.layout, nullptr);
    s.pipeline = VK_NULL_HANDLE;
    s.layout   = VK_NULL_HANDLE;
    pipelines_.Free(handle.id);
}

RHIRenderPass VulkanRHIDevice::CreateRenderPass(const RHIRenderPassDesc& desc) {
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorRefs;

    for (uint32_t i = 0; i < desc.colorAttachmentCount; ++i) {
        VkAttachmentDescription att{};
        att.format  = ToVkFormat(desc.colorAttachments[i].format);
        att.samples = VK_SAMPLE_COUNT_1_BIT;
        att.loadOp  = (desc.colorAttachments[i].loadOp == RHILoadOp::Clear)
            ? VK_ATTACHMENT_LOAD_OP_CLEAR
            : (desc.colorAttachments[i].loadOp == RHILoadOp::Load)
                ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att.storeOp = (desc.colorAttachments[i].storeOp == RHIStoreOp::Store)
            ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att.initialLayout = (desc.colorAttachments[i].loadOp == RHILoadOp::Load)
            ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
        att.finalLayout = desc.colorAttachments[i].sampleAfterPass
            ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference ref{};
        ref.attachment = static_cast<uint32_t>(attachments.size());
        ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachments.push_back(att);
        colorRefs.push_back(ref);
    }

    VkAttachmentReference depthRef{};
    if (desc.hasDepth) {
        VkAttachmentDescription att{};
        att.format  = ToVkFormat(desc.depthFormat);
        att.samples = VK_SAMPLE_COUNT_1_BIT;
        att.loadOp  = (desc.depthLoadOp == RHILoadOp::Clear)
            ? VK_ATTACHMENT_LOAD_OP_CLEAR
            : (desc.depthLoadOp == RHILoadOp::Load)
                ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att.storeOp = (desc.depthStoreOp == RHIStoreOp::Store)
            ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        att.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depthRef.attachment = static_cast<uint32_t>(attachments.size());
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(att);
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
    subpass.pColorAttachments    = colorRefs.data();
    subpass.pDepthStencilAttachment = desc.hasDepth ? &depthRef : nullptr;

    // Check if any attachment needs to transition for shader sampling
    bool needsSampleTransition = false;
    for (uint32_t i = 0; i < desc.colorAttachmentCount; ++i)
        needsSampleTransition |= desc.colorAttachments[i].sampleAfterPass;

    VkSubpassDependency deps[2] = {};
    uint32_t depCount = 1;

    deps[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass    = 0;
    deps[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                          | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    deps[0].dstStageMask  = deps[0].srcStageMask;
    deps[0].srcAccessMask = 0;
    deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                          | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    if (needsSampleTransition) {
        // Ensure color writes complete before fragment shader reads in a later pass
        deps[1].srcSubpass    = 0;
        deps[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
        deps[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[1].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        depCount = 2;
    }

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    rpInfo.pAttachments    = attachments.data();
    rpInfo.subpassCount    = 1;
    rpInfo.pSubpasses      = &subpass;
    rpInfo.dependencyCount = depCount;
    rpInfo.pDependencies   = deps;

    RenderPassSlot slot{};
    if (vkCreateRenderPass(device_, &rpInfo, nullptr, &slot.pass) != VK_SUCCESS) {
        KL_ERR("RHI", "Failed to create render pass");
        return {};
    }
    return {renderPasses_.Alloc(slot)};
}

void VulkanRHIDevice::DestroyRenderPass(RHIRenderPass handle) {
    if (!handle.IsValid() || !renderPasses_.Valid(handle.id)) return;
    auto& s = renderPasses_.Get(handle.id);
    if (s.pass != VK_NULL_HANDLE) vkDestroyRenderPass(device_, s.pass, nullptr);
    s.pass = VK_NULL_HANDLE;
    renderPasses_.Free(handle.id);
}

RHIFramebuffer VulkanRHIDevice::CreateFramebuffer(RHIRenderPass pass,
                                                    const RHITexture* colorAttachments,
                                                    uint32_t colorCount,
                                                    RHITexture depthAttachment,
                                                    uint32_t width, uint32_t height) {
    if (!pass.IsValid() || !renderPasses_.Valid(pass.id)) return {};

    std::vector<VkImageView> views;
    views.reserve(colorCount + 1);

    for (uint32_t i = 0; i < colorCount; ++i) {
        if (!colorAttachments[i].IsValid() || !textures_.Valid(colorAttachments[i].id))
            return {};
        views.push_back(textures_.Get(colorAttachments[i].id).view);
    }
    if (depthAttachment.IsValid() && textures_.Valid(depthAttachment.id))
        views.push_back(textures_.Get(depthAttachment.id).view);

    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass      = renderPasses_.Get(pass.id).pass;
    fbInfo.attachmentCount = static_cast<uint32_t>(views.size());
    fbInfo.pAttachments    = views.data();
    fbInfo.width           = width;
    fbInfo.height          = height;
    fbInfo.layers          = 1;

    FramebufferSlot slot{};
    slot.width  = width;
    slot.height = height;
    if (vkCreateFramebuffer(device_, &fbInfo, nullptr, &slot.framebuffer) != VK_SUCCESS) {
        KL_ERR("RHI", "Failed to create framebuffer");
        return {};
    }
    return {framebuffers_.Alloc(slot)};
}

void VulkanRHIDevice::DestroyFramebuffer(RHIFramebuffer handle) {
    if (!handle.IsValid() || !framebuffers_.Valid(handle.id)) return;
    auto& s = framebuffers_.Get(handle.id);
    if (s.framebuffer != VK_NULL_HANDLE)
        vkDestroyFramebuffer(device_, s.framebuffer, nullptr);
    s.framebuffer = VK_NULL_HANDLE;
    framebuffers_.Free(handle.id);
}

// -- Data transfer ---------------------------------------------------

void VulkanRHIDevice::UpdateBuffer(RHIBuffer handle, const void* data,
                                    size_t size, size_t offset) {
    if (!handle.IsValid() || !buffers_.Valid(handle.id)) return;
    auto& s = buffers_.Get(handle.id);

    if (s.hostVisible) {
        // For uniform buffers during an active frame, also write to the ring
        // buffer so each draw gets its own slice of data.
        if (s.isUniform && frameActive_ && uniformRingMapped_) {
            uint32_t aligned = (uniformRingOffset_ + uniformMinAlign_ - 1)
                             & ~(uniformMinAlign_ - 1);
            if (aligned + size <= uniformRingSize_) {
                std::memcpy(static_cast<uint8_t*>(uniformRingMapped_) + aligned,
                            data, size);
                s.ringOffset = aligned;
                uniformRingOffset_ = static_cast<uint32_t>(aligned + size);
            }
            // Fall through to also write to the original buffer so that on
            // frames where this buffer is NOT UpdateBuffer'd, the fallback
            // path (ringOffset == UINT32_MAX) reads valid data.
        }
        void* mapped = nullptr;
        VkResult mapResult = vkMapMemory(device_, s.memory, offset, size, 0, &mapped);
        if (mapResult == VK_SUCCESS && mapped) {
            std::memcpy(mapped, data, size);
            vkUnmapMemory(device_, s.memory);
        }
        if (!s.isUniform || s.ringOffset == UINT32_MAX)
            s.ringOffset = UINT32_MAX; // not ring-backed
    } else {
        // Staging upload
        VkBuffer staging;
        VkDeviceMemory stagingMem;
        if (!CreateVkBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            staging, stagingMem))
            return;

        void* mapped = nullptr;
        vkMapMemory(device_, stagingMem, 0, size, 0, &mapped);
        std::memcpy(mapped, data, size);
        vkUnmapMemory(device_, stagingMem);

        auto cmd = BeginOneShot();
        VkBufferCopy region{};
        region.srcOffset = 0;
        region.dstOffset = offset;
        region.size = size;
        vkCmdCopyBuffer(cmd, staging, s.buffer, 1, &region);
        EndOneShot(cmd);

        vkDestroyBuffer(device_, staging, nullptr);
        vkFreeMemory(device_, stagingMem, nullptr);
    }
}

void VulkanRHIDevice::UpdateTexture(RHITexture handle, const void* data,
                                     size_t dataSize,
                                     uint32_t width, uint32_t height) {
    if (!handle.IsValid() || !textures_.Valid(handle.id)) return;
    auto& s = textures_.Get(handle.id);

    // Create staging buffer
    VkBuffer staging;
    VkDeviceMemory stagingMem;
    if (!CreateVkBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        staging, stagingMem))
        return;

    void* mapped = nullptr;
    vkMapMemory(device_, stagingMem, 0, dataSize, 0, &mapped);
    std::memcpy(mapped, data, dataSize);
    vkUnmapMemory(device_, stagingMem);

    auto cmd = BeginOneShot();

    TransitionImageLayout(cmd, s.image, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_ASPECT_COLOR_BIT);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(cmd, staging, s.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    TransitionImageLayout(cmd, s.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_IMAGE_ASPECT_COLOR_BIT);

    EndOneShot(cmd);

    vkDestroyBuffer(device_, staging, nullptr);
    vkFreeMemory(device_, stagingMem, nullptr);
}

void* VulkanRHIDevice::MapBuffer(RHIBuffer handle) {
    if (!handle.IsValid() || !buffers_.Valid(handle.id)) return nullptr;
    auto& s = buffers_.Get(handle.id);
    if (!s.hostVisible) return nullptr;
    if (s.mapped) return s.mapped;
    vkMapMemory(device_, s.memory, 0, s.size, 0, &s.mapped);
    return s.mapped;
}

void VulkanRHIDevice::UnmapBuffer(RHIBuffer handle) {
    if (!handle.IsValid() || !buffers_.Valid(handle.id)) return;
    auto& s = buffers_.Get(handle.id);
    if (s.mapped) {
        vkUnmapMemory(device_, s.memory);
        s.mapped = nullptr;
    }
}

// -- Command recording -----------------------------------------------

void VulkanRHIDevice::BeginFrame() {
    // Acquire the display's command buffer. This waits on the current
    // frame's fence, guaranteeing that the GPU has finished executing
    // this frame slot's previous command buffer. After this point,
    // resources used by that prior submission are safe to reclaim.
    cmdBuffer_ = display_->BeginFrame();
    if (!cmdBuffer_) return;

    // Advance to this frame's descriptor pool (matches display's double-buffering)
    currentPoolIndex_ = display_->GetCurrentFrame() % kMaxFramesInFlight;

    // Flush deferred texture deletions queued during THIS frame slot's
    // previous use. The fence wait above guarantees completeness.
    auto& texDeletions = pendingTextureDeletes_[currentPoolIndex_];
    for (auto& s : texDeletions) {
        if (s.sampler != VK_NULL_HANDLE) vkDestroySampler(device_, s.sampler, nullptr);
        if (s.view != VK_NULL_HANDLE)    vkDestroyImageView(device_, s.view, nullptr);
        if (s.image != VK_NULL_HANDLE)   vkDestroyImage(device_, s.image, nullptr);
        if (s.memory != VK_NULL_HANDLE)  vkFreeMemory(device_, s.memory, nullptr);
    }
    texDeletions.clear();

    // Flush deferred buffer deletions (same lifecycle as textures).
    auto& bufDeletions = pendingBufferDeletes_[currentPoolIndex_];
    for (auto& s : bufDeletions) {
        if (s.mapped)  vkUnmapMemory(device_, s.memory);
        if (s.buffer != VK_NULL_HANDLE) vkDestroyBuffer(device_, s.buffer, nullptr);
        if (s.memory != VK_NULL_HANDLE) vkFreeMemory(device_, s.memory, nullptr);
    }
    bufDeletions.clear();

    // Reset this frame slot's descriptor pool (safe - fence waited above).
    vkResetDescriptorPool(device_, descriptorPools_[currentPoolIndex_], 0);

    // Reset active pipeline layout so the first BindUniformBuffer of the
    // new frame defaults to scenePipelineLayout_ (not the previous frame's
    // blit layout, which would cause a descriptor type mismatch).
    activePipelineLayout_ = VK_NULL_HANDLE;

    // Invalidate cached scene set 0 (pool was reset, old sets are gone).
    cachedSceneSet0_ = VK_NULL_HANDLE;
    sceneSet0Written_ = 0;
    sceneSet0Dirty_ = false;
    sceneSet0Flushed_ = false;

    // Reset uniform ring buffer write head for this frame.
    uniformRingOffset_ = 0;
    // Clear per-buffer ring offsets so that buffers not UpdateBuffer'd
    // this frame fall back to reading from their original VkBuffer.
    for (size_t i = 1; i < buffers_.slots.size(); ++i) {
        buffers_.slots[i].ringOffset = UINT32_MAX;
    }

    frameActive_ = true;
}

void VulkanRHIDevice::BeginRenderPass(RHIRenderPass pass,
                                       RHIFramebuffer framebuffer,
                                       const RHIClearValue& clear) {
    if (!pass.IsValid() || !framebuffer.IsValid()) return;
    if (!renderPasses_.Valid(pass.id) || !framebuffers_.Valid(framebuffer.id)) return;

    auto& fb = framebuffers_.Get(framebuffer.id);

    VkClearValue clearValues[2]{};
    clearValues[0].color = {{clear.color[0], clear.color[1],
                              clear.color[2], clear.color[3]}};
    clearValues[1].depthStencil = {clear.depth, clear.stencil};

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass  = renderPasses_.Get(pass.id).pass;
    rpInfo.framebuffer = fb.framebuffer;
    rpInfo.renderArea  = {{0, 0}, {fb.width, fb.height}};
    rpInfo.clearValueCount = 2;
    rpInfo.pClearValues    = clearValues;

    vkCmdBeginRenderPass(cmdBuffer_, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRHIDevice::EndRenderPass() {
    vkCmdEndRenderPass(cmdBuffer_);
}

void VulkanRHIDevice::BindPipeline(RHIPipeline pipeline) {
    if (!pipeline.IsValid() || !pipelines_.Valid(pipeline.id)) return;
    auto& slot = pipelines_.Get(pipeline.id);
    vkCmdBindPipeline(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, slot.pipeline);

    // Invalidate the cached scene set 0 when the EFFECTIVE pipeline layout
    // changes (e.g. scene - blit or vice-versa).  VK_NULL_HANDLE defaults
    // to scenePipelineLayout_ in BindUniformBuffer, so a null - scene
    // transition is NOT a real layout change and must not invalidate.
    VkPipelineLayout effectiveOld = (activePipelineLayout_ != VK_NULL_HANDLE)
                                  ? activePipelineLayout_
                                  : scenePipelineLayout_;
    if (slot.layout != effectiveOld) {
        cachedSceneSet0_ = VK_NULL_HANDLE;
        sceneSet0Written_ = 0;
        sceneSet0Dirty_ = false;
        sceneSet0Flushed_ = false;
    }
    activePipelineLayout_ = slot.layout;
}

void VulkanRHIDevice::BindVertexBuffer(RHIBuffer buffer,
                                        uint32_t binding, uint64_t offset) {
    if (!buffer.IsValid() || !buffers_.Valid(buffer.id)) return;
    VkBuffer vkBuf = buffers_.Get(buffer.id).buffer;
    VkDeviceSize off = offset;
    vkCmdBindVertexBuffers(cmdBuffer_, binding, 1, &vkBuf, &off);
}

void VulkanRHIDevice::BindIndexBuffer(RHIBuffer buffer,
                                       bool is32Bit, uint64_t offset) {
    if (!buffer.IsValid() || !buffers_.Valid(buffer.id)) return;
    vkCmdBindIndexBuffer(cmdBuffer_, buffers_.Get(buffer.id).buffer, offset,
                         is32Bit ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
}

void VulkanRHIDevice::BindUniformBuffer(RHIBuffer buffer, uint32_t set,
                                         uint32_t binding,
                                         size_t offset, size_t range) {
    if (!buffer.IsValid() || !buffers_.Valid(buffer.id)) return;

    VkPipelineLayout pipeLayout = activePipelineLayout_;
    if (pipeLayout == VK_NULL_HANDLE) pipeLayout = scenePipelineLayout_;

    // Select the correct descriptor set layout for this set index.
    // For scene layout: set 0=scene, 1=material, 2=textures
    // For blit layout:  set 0=sampler, 1=UBO
    VkDescriptorSetLayout dsLayout = VK_NULL_HANDLE;
    VkDescriptorType descType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bool useSceneSet0Cache = false;

    if (pipeLayout == blitPipelineLayout_) {
        dsLayout = (set == 0) ? blitSamplerLayout_ : blitUBOLayout_;
    } else {
        // Scene pipeline - determine type by binding:
        //   Set 0, Binding 1 = SSBO (lights), Binding 3 = SSBO (audio)
        //   Everything else = UBO
        if (set == 0) {
            dsLayout = sceneSetLayout_;
            useSceneSet0Cache = true;
            if (binding == 1 || binding == 3)
                descType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        } else if (set == 1) {
            dsLayout = materialSetLayout_;
        } else {
            dsLayout = textureSetLayout_;
        }
    }

    // -- Scene set 0: write-accumulate with deferred bind --------------
    //
    // sceneSetLayout_ has 4 bindings (transform, lights, scene, audio)
    // that are written by separate BindUniformBuffer calls.  We accumulate
    // writes into one descriptor set and defer vkCmdBindDescriptorSets
    // until the next Draw/DrawIndexed call (see FlushPendingDescriptorBinds).
    //
    // Per-mesh re-binds of the same buffer are skipped entirely: the
    // descriptor already references the buffer and only its CONTENT
    // changes (via UpdateBuffer), which doesn't require a descriptor update.
    if (useSceneSet0Cache) {
        if (cachedSceneSet0_ == VK_NULL_HANDLE) {
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptorPools_[currentPoolIndex_];
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &dsLayout;

            if (vkAllocateDescriptorSets(device_, &allocInfo, &cachedSceneSet0_) != VK_SUCCESS) {
                KL_ERR("RHI", "Failed to allocate scene set 0");
                return;
            }
            sceneSet0Written_ = 0;
            sceneSet0Dirty_ = true;
            sceneSet0Flushed_ = false;
        }

        // Store binding info (indexed by binding number).  All writes are
        // deferred to FlushPendingDescriptorBinds().
        //
        // Ring-backed uniform buffers (those written to the ring buffer by
        // UpdateBuffer) ALWAYS update here because the ring offset changes
        // per draw.  Non-ring bindings (lights, scene globals, audio) are
        // written once and then skipped via the bitmask.
        auto& s = buffers_.Get(buffer.id);
        bool usesRing = (s.isUniform && s.ringOffset != UINT32_MAX);

        if (usesRing || !(sceneSet0Written_ & (1u << binding))) {
            auto& sb = sceneSet0Bindings_[binding];
            sb.descType = descType;
            if (usesRing) {
                sb.bufInfo.buffer = uniformRingBuffer_;
                sb.bufInfo.offset = s.ringOffset;
                sb.bufInfo.range  = s.size;
            } else {
                sb.bufInfo.buffer = s.buffer;
                sb.bufInfo.offset = offset;
                sb.bufInfo.range  = (range > 0)
                                  ? static_cast<VkDeviceSize>(range) : s.size;
            }

            sceneSet0Written_ |= (1u << binding);
            sceneSet0Dirty_ = true;
        }

        return; // Bind deferred to FlushPendingDescriptorBinds()
    }

    // -- All other sets: allocate-write-bind per call ------------------

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPools_[currentPoolIndex_];
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &dsLayout;

    VkDescriptorSet descSet;
    if (vkAllocateDescriptorSets(device_, &allocInfo, &descSet) != VK_SUCCESS) {
        KL_ERR("RHI", "Failed to allocate descriptor set (set=%u, binding=%u)", set, binding);
        return;
    }

    auto& s = buffers_.Get(buffer.id);
    VkDescriptorBufferInfo bufInfo{};
    if (s.isUniform && s.ringOffset != UINT32_MAX) {
        bufInfo.buffer = uniformRingBuffer_;
        bufInfo.offset = s.ringOffset;
        bufInfo.range  = s.size;
    } else {
        bufInfo.buffer = s.buffer;
        bufInfo.offset = offset;
        bufInfo.range  = (range > 0) ? range : s.size;
    }

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descSet;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = descType;
    write.pBufferInfo = &bufInfo;
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

    vkCmdBindDescriptorSets(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeLayout, set, 1, &descSet, 0, nullptr);
}

void VulkanRHIDevice::BindTexture(RHITexture texture,
                                   uint32_t set, uint32_t binding) {
    if (!texture.IsValid() || !textures_.Valid(texture.id)) return;

    VkPipelineLayout pipeLayout = activePipelineLayout_;
    if (pipeLayout == VK_NULL_HANDLE) pipeLayout = scenePipelineLayout_;

    // Select the correct descriptor set layout for texture binding
    VkDescriptorSetLayout dsLayout = VK_NULL_HANDLE;
    if (pipeLayout == blitPipelineLayout_) {
        dsLayout = blitSamplerLayout_;
    } else {
        dsLayout = textureSetLayout_;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPools_[currentPoolIndex_];
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &dsLayout;

    VkDescriptorSet descSet;
    if (vkAllocateDescriptorSets(device_, &allocInfo, &descSet) != VK_SUCCESS) {
        KL_ERR("RHI", "Failed to allocate texture descriptor set");
        return;
    }

    auto& s = textures_.Get(texture.id);
    VkDescriptorImageInfo imgInfo{};
    imgInfo.sampler     = s.sampler;
    imgInfo.imageView   = s.view;
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descSet;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imgInfo;
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

    vkCmdBindDescriptorSets(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeLayout, set, 1, &descSet, 0, nullptr);
}

void VulkanRHIDevice::SetViewport(const RHIViewport& vp) {
    VkViewport viewport{};
    viewport.x        = vp.x;
    viewport.y        = vp.y;
    viewport.width    = vp.width;
    viewport.height   = vp.height;
    viewport.minDepth = vp.minDepth;
    viewport.maxDepth = vp.maxDepth;
    vkCmdSetViewport(cmdBuffer_, 0, 1, &viewport);
}

void VulkanRHIDevice::SetScissor(const RHIScissor& sc) {
    VkRect2D scissor{};
    scissor.offset = {sc.x, sc.y};
    scissor.extent = {sc.width, sc.height};
    vkCmdSetScissor(cmdBuffer_, 0, 1, &scissor);
}

void VulkanRHIDevice::PushConstants(RHIShaderStage stages, const void* data,
                                     uint32_t size, uint32_t offset) {
    VkShaderStageFlags vkStages = 0;
    if (static_cast<uint8_t>(stages) & static_cast<uint8_t>(RHIShaderStage::Vertex))
        vkStages |= VK_SHADER_STAGE_VERTEX_BIT;
    if (static_cast<uint8_t>(stages) & static_cast<uint8_t>(RHIShaderStage::Fragment))
        vkStages |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (static_cast<uint8_t>(stages) & static_cast<uint8_t>(RHIShaderStage::Compute))
        vkStages |= VK_SHADER_STAGE_COMPUTE_BIT;

    VkPipelineLayout pipeLayout = activePipelineLayout_;
    if (pipeLayout == VK_NULL_HANDLE) pipeLayout = scenePipelineLayout_;
    vkCmdPushConstants(cmdBuffer_, pipeLayout, vkStages, offset, size, data);
}

void VulkanRHIDevice::FlushPendingDescriptorBinds() {
    if (cachedSceneSet0_ == VK_NULL_HANDLE || !sceneSet0Dirty_) return;

    // If the set was already flushed (written+bound to the command buffer),
    // we CANNOT call vkUpdateDescriptorSets on it - that violates the
    // UPDATE_AFTER_BIND rule.  Allocate a fresh set instead.
    if (sceneSet0Flushed_) {
        VkDescriptorSet freshSet = VK_NULL_HANDLE;
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPools_[currentPoolIndex_];
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &sceneSetLayout_;
        if (vkAllocateDescriptorSets(device_, &allocInfo, &freshSet) != VK_SUCCESS) {
            KL_ERR("RHI", "Failed to re-allocate scene set 0 for deferred writes");
            return;
        }
        cachedSceneSet0_ = freshSet;
    }

    // Write ALL stored bindings to the (possibly fresh) descriptor set.
    uint32_t writeCount = 0;
    VkWriteDescriptorSet writes[kMaxSceneSet0Bindings];
    for (uint32_t b = 0; b < kMaxSceneSet0Bindings; ++b) {
        if (!(sceneSet0Written_ & (1u << b))) continue;
        auto& sb = sceneSet0Bindings_[b];
        auto& w  = writes[writeCount];
        w = {};
        w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet          = cachedSceneSet0_;
        w.dstBinding      = b;
        w.descriptorCount = 1;
        w.descriptorType  = sb.descType;
        w.pBufferInfo     = &sb.bufInfo;
        ++writeCount;
    }
    if (writeCount > 0) {
        vkUpdateDescriptorSets(device_, writeCount, writes, 0, nullptr);
    }

    VkPipelineLayout pipeLayout = activePipelineLayout_;
    if (pipeLayout == VK_NULL_HANDLE) pipeLayout = scenePipelineLayout_;
    vkCmdBindDescriptorSets(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeLayout, 0, 1, &cachedSceneSet0_, 0, nullptr);
    sceneSet0Dirty_ = false;
    sceneSet0Flushed_ = true;
}

void VulkanRHIDevice::Draw(uint32_t vertexCount, uint32_t instanceCount,
                            uint32_t firstVertex, uint32_t firstInstance) {
    FlushPendingDescriptorBinds();
    vkCmdDraw(cmdBuffer_, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanRHIDevice::DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                   uint32_t firstIndex, int32_t vertexOffset,
                                   uint32_t firstInstance) {
    FlushPendingDescriptorBinds();
    vkCmdDrawIndexed(cmdBuffer_, indexCount, instanceCount, firstIndex,
                     vertexOffset, firstInstance);
}

void VulkanRHIDevice::EndFrame() {
    // No-op: the display's SwapOnly() handles command buffer ending,
    // queue submission with semaphore synchronization, and presentation.
    // We just clear our frame-active flag so descriptor pool is recycled.
    frameActive_ = false;
}

void VulkanRHIDevice::Present() {
    // Submit + present via the display backend.
    if (display_) display_->SwapOnly();
}

// -- Swapchain -------------------------------------------------------

void VulkanRHIDevice::GetSwapchainSize(uint32_t& outWidth,
                                        uint32_t& outHeight) const {
    if (display_) {
        outWidth  = display_->GetSwapchainWidth();
        outHeight = display_->GetSwapchainHeight();
    } else {
        outWidth = outHeight = 0;
    }
}

void VulkanRHIDevice::OnResize(uint32_t /*width*/, uint32_t /*height*/) {
    // VulkanBackend handles swapchain recreation internally.
    // This hook is available for future per-resize resource updates.
}

void VulkanRHIDevice::BeginSwapchainRenderPass(const RHIClearValue& clear) {
    (void)clear;  // Display's render pass uses its own clear values
    if (!display_) return;

    // Delegate to the display's swapchain render pass.
    // The display must be in an active frame (BeginFrame already called).
    display_->BeginSwapchainRenderPass();
}

RHIRenderPass VulkanRHIDevice::GetSwapchainRenderPass() const {
    if (swapchainPassHandle_.IsValid()) return swapchainPassHandle_;
    if (!display_) return {};

    // Wrap the display's native VkRenderPass in an RHI handle.
    // The slot is NOT destroyed during Shutdown because the VkRenderPass
    // is owned by the display backend, not by us.
    VkRenderPass nativePass = display_->GetRenderPass();
    if (nativePass == VK_NULL_HANDLE) return {};

    RenderPassSlot slot{};
    slot.pass = nativePass;
    swapchainPassHandle_ = {const_cast<VulkanRHIDevice*>(this)->renderPasses_.Alloc(slot)};
    return swapchainPassHandle_;
}

} // namespace koilo::rhi
