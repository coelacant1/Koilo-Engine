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

    // Create descriptor pool
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         256},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  256},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,          64},
    };
    VkDescriptorPoolCreateInfo dpInfo{};
    dpInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    dpInfo.maxSets = 512;
    dpInfo.poolSizeCount = 3;
    dpInfo.pPoolSizes = poolSizes;
    if (vkCreateDescriptorPool(device_, &dpInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
        KL_ERR("RHI", "Failed to create descriptor pool");
        return false;
    }

    // Create descriptor set layouts
    // Set 0: single uniform buffer (scene/transform data)
    {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;
        if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &uniformLayout_) != VK_SUCCESS) {
            KL_ERR("RHI", "Failed to create uniform descriptor set layout");
            return false;
        }
    }
    // Set 1: single combined image sampler
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
        if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &textureLayout_) != VK_SUCCESS) {
            KL_ERR("RHI", "Failed to create texture descriptor set layout");
            return false;
        }
    }

    // Create default pipeline layout (set 0 = uniform, set 1 = texture, push constants)
    {
        VkDescriptorSetLayout layouts[] = {uniformLayout_, textureLayout_};

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
                                    &defaultPipelineLayout_) != VK_SUCCESS) {
            KL_ERR("RHI", "Failed to create default pipeline layout");
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

    // Destroy infrastructure
    if (defaultPipelineLayout_ != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device_, defaultPipelineLayout_, nullptr);
    if (uniformLayout_ != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device_, uniformLayout_, nullptr);
    if (textureLayout_ != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device_, textureLayout_, nullptr);
    if (descriptorPool_ != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
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

    if (!CreateVkBuffer(desc.size, vkUsage, memProps, slot.buffer, slot.memory)) {
        KL_ERR("RHI", "Failed to create buffer (%s)", desc.debugName ? desc.debugName : "unnamed");
        return {};
    }

    return {buffers_.Alloc(slot)};
}

void VulkanRHIDevice::DestroyBuffer(RHIBuffer handle) {
    if (!handle.IsValid() || !buffers_.Valid(handle.id)) return;
    auto& s = buffers_.Get(handle.id);
    if (s.mapped) { vkUnmapMemory(device_, s.memory); s.mapped = nullptr; }
    if (s.buffer != VK_NULL_HANDLE) vkDestroyBuffer(device_, s.buffer, nullptr);
    if (s.memory != VK_NULL_HANDLE) vkFreeMemory(device_, s.memory, nullptr);
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
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
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
    if (s.sampler != VK_NULL_HANDLE) vkDestroySampler(device_, s.sampler, nullptr);
    if (s.view != VK_NULL_HANDLE)    vkDestroyImageView(device_, s.view, nullptr);
    if (s.image != VK_NULL_HANDLE)   vkDestroyImage(device_, s.image, nullptr);
    if (s.memory != VK_NULL_HANDLE)  vkFreeMemory(device_, s.memory, nullptr);
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
    pipelineInfo.layout              = defaultPipelineLayout_;
    pipelineInfo.renderPass          = vkPass;
    pipelineInfo.subpass             = 0;

    PipelineSlot slot{};
    slot.layout = defaultPipelineLayout_;
    slot.ownsLayout = false;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo,
                                   nullptr, &slot.pipeline) != VK_SUCCESS) {
        KL_ERR("RHI", "Failed to create pipeline (%s)",
               desc.debugName ? desc.debugName : "unnamed");
        return {};
    }

    return {pipelines_.Alloc(slot)};
}

void VulkanRHIDevice::DestroyPipeline(RHIPipeline handle) {
    if (!handle.IsValid() || !pipelines_.Valid(handle.id)) return;
    auto& s = pipelines_.Get(handle.id);
    if (s.pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device_, s.pipeline, nullptr);
    if (s.ownsLayout && s.layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device_, s.layout, nullptr);
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
        att.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                      | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask  = dep.srcStageMask;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                      | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    rpInfo.pAttachments    = attachments.data();
    rpInfo.subpassCount    = 1;
    rpInfo.pSubpasses      = &subpass;
    rpInfo.dependencyCount = 1;
    rpInfo.pDependencies   = &dep;

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
    framebuffers_.Free(handle.id);
}

// -- Data transfer ---------------------------------------------------

void VulkanRHIDevice::UpdateBuffer(RHIBuffer handle, const void* data,
                                    size_t size, size_t offset) {
    if (!handle.IsValid() || !buffers_.Valid(handle.id)) return;
    auto& s = buffers_.Get(handle.id);

    if (s.hostVisible) {
        void* mapped = nullptr;
        vkMapMemory(device_, s.memory, offset, size, 0, &mapped);
        std::memcpy(mapped, data, size);
        vkUnmapMemory(device_, s.memory);
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
    vkWaitForFences(device_, 1, &frameFence_, VK_TRUE, UINT64_MAX);
    vkResetFences(device_, 1, &frameFence_);

    // Reclaim all descriptor sets allocated during the previous frame.
    vkResetDescriptorPool(device_, descriptorPool_, 0);

    vkResetCommandBuffer(cmdBuffer_, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmdBuffer_, &beginInfo);
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
    vkCmdBindPipeline(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipelines_.Get(pipeline.id).pipeline);
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

    // Allocate a descriptor set from the pool
    VkDescriptorSetLayout layout = (set == 0) ? uniformLayout_ : uniformLayout_;
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descSet;
    if (vkAllocateDescriptorSets(device_, &allocInfo, &descSet) != VK_SUCCESS) {
        KL_ERR("RHI", "Failed to allocate uniform descriptor set");
        return;
    }

    auto& s = buffers_.Get(buffer.id);
    VkDescriptorBufferInfo bufInfo{};
    bufInfo.buffer = s.buffer;
    bufInfo.offset = offset;
    bufInfo.range  = (range > 0) ? range : s.size;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descSet;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &bufInfo;
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

    vkCmdBindDescriptorSets(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            defaultPipelineLayout_, set, 1, &descSet, 0, nullptr);
}

void VulkanRHIDevice::BindTexture(RHITexture texture,
                                   uint32_t set, uint32_t binding) {
    if (!texture.IsValid() || !textures_.Valid(texture.id)) return;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &textureLayout_;

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
                            defaultPipelineLayout_, set, 1, &descSet, 0, nullptr);
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

    vkCmdPushConstants(cmdBuffer_, defaultPipelineLayout_, vkStages, offset, size, data);
}

void VulkanRHIDevice::Draw(uint32_t vertexCount, uint32_t instanceCount,
                            uint32_t firstVertex, uint32_t firstInstance) {
    vkCmdDraw(cmdBuffer_, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanRHIDevice::DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                   uint32_t firstIndex, int32_t vertexOffset,
                                   uint32_t firstInstance) {
    vkCmdDrawIndexed(cmdBuffer_, indexCount, instanceCount, firstIndex,
                     vertexOffset, firstInstance);
}

void VulkanRHIDevice::EndFrame() {
    if (!frameActive_) return;
    vkEndCommandBuffer(cmdBuffer_);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer_;

    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, frameFence_);
    frameActive_ = false;
}

void VulkanRHIDevice::Present() {
    // Presentation is delegated to VulkanBackend display layer
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

} // namespace koilo::rhi
