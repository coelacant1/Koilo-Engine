// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file vulkan_render_backend.cpp
 * @brief Vulkan GPU render backend implementation.
 *
 * Renders scenes to an off-screen render target using Vulkan pipelines,
 * descriptor sets, and SPIR-V shaders from the KSL pipeline.
 *
 * @date 02/18/2026
 * @author Coela
 */

#include <koilo/systems/render/vk/vulkan_render_backend.hpp>
#include <koilo/systems/display/backends/gpu/vulkanbackend.hpp>
#include <koilo/systems/scene/scene.hpp>
#include <koilo/systems/scene/mesh.hpp>
#include <koilo/systems/scene/camera/camerabase.hpp>
#include <koilo/systems/scene/camera/camera.hpp>
#include <koilo/systems/scene/camera/cameralayout.hpp>
#include <koilo/systems/render/sky/sky.hpp>
#include <koilo/systems/render/material/imaterial.hpp>
#include <koilo/systems/render/material/implementations/kslmaterial.hpp>
#include <koilo/ksl/ksl_symbols.hpp>
#include <koilo/systems/scene/lighting/light.hpp>
#include <koilo/assets/image/texture.hpp>
#include <koilo/core/math/matrix4x4.hpp>
#include <koilo/core/math/transform.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/vector2d.hpp>
#include <koilo/assets/model/itrianglegroup.hpp>
#include <koilo/core/geometry/3d/triangle.hpp>
#include <koilo/assets/model/indexgroup.hpp>
#include <koilo/systems/profiling/performanceprofiler.hpp>
#include <koilo/debug/debugdraw.hpp>
#include <koilo/systems/render/canvas2d.hpp>
#include <koilo/systems/render/render_cvars.hpp>
#include <koilo/kernel/logging/log.hpp>

#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <array>

namespace koilo {

// ============================================================================
// Embedded SPIR-V for blit/overlay/line shaders (generated from simple GLSL)
// We compile these inline as uint32_t arrays to avoid external file deps.
// ============================================================================

// Fullscreen triangle vertex shader (no VBO needed, but we use a quad VBO)
// #version 450
// layout(location=0) in vec2 a_position;
// layout(location=1) in vec2 a_uv;
// layout(location=0) out vec2 v_uv;
// void main() { v_uv = a_uv; gl_Position = vec4(a_position, 0.0, 1.0); }
//
// Fragment shader for blit/overlay:
// #version 450
// layout(location=0) in vec2 v_uv;
// layout(set=0, binding=0) uniform sampler2D u_texture;
// layout(location=0) out vec4 fragColor;
// void main() { fragColor = texture(u_texture, v_uv); }

// We'll compile these at init time from GLSL source using VkShaderModule
// For now, we use runtime GLSL->SPIR-V is not available in Vulkan, so we
// store minimal SPIR-V bytecode. These are generated offline.
//
// Instead, we'll create these shaders at build time via CMake.
// For the initial implementation, we use the off-screen render target
// and blit via a memory copy approach (simpler, works everywhere).

// ============================================================================
// Quad vertex data (pos2 + uv2)
// ============================================================================

struct QuadVertex {
    float px, py;
    float u, v;
};

// Non-flipped UVs (blit: v=0 at bottom, matching Vulkan convention)
static const QuadVertex s_blitQuadVerts[] = {
    {-1.0f, -1.0f,  0.0f, 0.0f},
    { 1.0f, -1.0f,  1.0f, 0.0f},
    { 1.0f,  1.0f,  1.0f, 1.0f},
    {-1.0f, -1.0f,  0.0f, 0.0f},
    { 1.0f,  1.0f,  1.0f, 1.0f},
    {-1.0f,  1.0f,  0.0f, 1.0f},
};

// Sky quad vertices (pos3, normal3, uv2 - matches standard vertex layout)
static const float s_skyQuadVerts[] = {
    -1.0f, -1.0f, 0.999f,  0,0,1,  0.0f, 0.0f,
     1.0f, -1.0f, 0.999f,  0,0,1,  1.0f, 0.0f,
     1.0f,  1.0f, 0.999f,  0,0,1,  1.0f, 1.0f,
    -1.0f, -1.0f, 0.999f,  0,0,1,  0.0f, 0.0f,
     1.0f,  1.0f, 0.999f,  0,0,1,  1.0f, 1.0f,
    -1.0f,  1.0f, 0.999f,  0,0,1,  0.0f, 1.0f,
};

// ============================================================================
// Constructor / Destructor
// ============================================================================

VulkanRenderBackend::VulkanRenderBackend(VulkanBackend* display)
    : display_(display) {}

VulkanRenderBackend::~VulkanRenderBackend() {
    Shutdown();
}

// ============================================================================
// Buffer Helpers
// ============================================================================

uint32_t VulkanRenderBackend::FindMemoryType(uint32_t typeFilter,
                                              VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physDevice_, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    return UINT32_MAX;
}

bool VulkanRenderBackend::CreateBuffer(VkDeviceSize size,
                                        VkBufferUsageFlags usage,
                                        VkMemoryPropertyFlags props,
                                        VkBuffer& buffer,
                                        VkDeviceMemory& memory) {
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size = size;
    bufInfo.usage = usage;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &bufInfo, nullptr, &buffer) != VK_SUCCESS)
        return false;

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device_, buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, props);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        vkDestroyBuffer(device_, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        return false;
    }

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyBuffer(device_, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        return false;
    }

    if (vkBindBufferMemory(device_, buffer, memory, 0) != VK_SUCCESS) {
        vkDestroyBuffer(device_, buffer, nullptr);
        vkFreeMemory(device_, memory, nullptr);
        buffer = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

bool VulkanRenderBackend::CreateBufferMapped(VkDeviceSize size,
                                              VkBufferUsageFlags usage,
                                              VkBuffer& buffer,
                                              VkDeviceMemory& memory,
                                              void** mapped) {
    if (!CreateBuffer(size, usage,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      buffer, memory))
        return false;
    if (vkMapMemory(device_, memory, 0, size, 0, mapped) != VK_SUCCESS) {
        DestroyBuffer(buffer, memory);
        return false;
    }
    return true;
}

void VulkanRenderBackend::DestroyBuffer(VkBuffer& buffer,
                                         VkDeviceMemory& memory) {
    if (buffer) { vkDestroyBuffer(device_, buffer, nullptr); buffer = VK_NULL_HANDLE; }
    if (memory) { vkFreeMemory(device_, memory, nullptr); memory = VK_NULL_HANDLE; }
}

// ============================================================================
// Image Helpers
// ============================================================================

bool VulkanRenderBackend::CreateImage(uint32_t w, uint32_t h, VkFormat fmt,
                                       VkImageUsageFlags usage,
                                       VkImage& img, VkDeviceMemory& mem) {
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

    if (vkCreateImage(device_, &info, nullptr, &img) != VK_SUCCESS)
        return false;

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device_, img, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        vkDestroyImage(device_, img, nullptr);
        img = VK_NULL_HANDLE;
        return false;
    }

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &mem) != VK_SUCCESS) {
        vkDestroyImage(device_, img, nullptr);
        img = VK_NULL_HANDLE;
        return false;
    }

    if (vkBindImageMemory(device_, img, mem, 0) != VK_SUCCESS) {
        vkDestroyImage(device_, img, nullptr);
        vkFreeMemory(device_, mem, nullptr);
        img = VK_NULL_HANDLE;
        mem = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

VkImageView VulkanRenderBackend::CreateImageView(VkImage image, VkFormat format,
                                                   VkImageAspectFlags aspect) {
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
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

void VulkanRenderBackend::TransitionImageLayout(VkCommandBuffer cmd,
                                                  VkImage image,
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
        newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
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
    } else {
        barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);
}

// ============================================================================
// One-shot command helpers
// ============================================================================

VkCommandBuffer VulkanRenderBackend::BeginOneShot() {
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

void VulkanRenderBackend::EndOneShot(VkCommandBuffer cmd) {
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    VkResult result = vkQueueSubmit(graphicsQueue_, 1, &submit, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        const char* errName = "UNKNOWN";
        switch (result) {
            case VK_ERROR_DEVICE_LOST: errName = "VK_ERROR_DEVICE_LOST"; break;
            case VK_ERROR_OUT_OF_HOST_MEMORY: errName = "VK_ERROR_OUT_OF_HOST_MEMORY"; break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: errName = "VK_ERROR_OUT_OF_DEVICE_MEMORY"; break;
            default: break;
        }
        KL_ERR("VulkanRender", "vkQueueSubmit failed in EndOneShot (%s, %d)",
               errName, (int)result);
    }
    vkQueueWaitIdle(graphicsQueue_);
    vkFreeCommandBuffers(device_, cmdPool_, 1, &cmd);
}

// ============================================================================
// Off-screen Render Target
// ============================================================================

bool VulkanRenderBackend::CreateOffscreenTarget(int width, int height) {
    DestroyOffscreenTarget();

    uint32_t w = static_cast<uint32_t>(width);
    uint32_t h = static_cast<uint32_t>(height);

    // Color attachment (RGBA8, used as color attachment + sampled for blit)
    if (!CreateImage(w, h, VK_FORMAT_R8G8B8A8_UNORM,
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                     VK_IMAGE_USAGE_SAMPLED_BIT |
                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                     colorImage_, colorMemory_))
        return false;

    colorView_ = CreateImageView(colorImage_, VK_FORMAT_R8G8B8A8_UNORM,
                                 VK_IMAGE_ASPECT_COLOR_BIT);
    if (!colorView_) return false;

    // Depth attachment
    VkFormat depthFmt = VK_FORMAT_D32_SFLOAT;
    if (!CreateImage(w, h, depthFmt,
                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                     depthImage_, depthMemory_))
        return false;

    depthView_ = CreateImageView(depthImage_, depthFmt,
                                 VK_IMAGE_ASPECT_DEPTH_BIT);
    if (!depthView_) return false;

    // Transition depth image to optimal layout
    auto cmd = BeginOneShot();
    TransitionImageLayout(cmd, depthImage_,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_ASPECT_DEPTH_BIT);
    EndOneShot(cmd);

    // Render pass (color + depth -> color stays in attachment optimal)
    if (!offscreenPass_) {
        VkAttachmentDescription attachments[2] = {};
        // Color
        attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // Depth
        attachments[1].format = depthFmt;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency dep{};
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                           VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                           VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.srcAccessMask = 0;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpInfo.attachmentCount = 2;
        rpInfo.pAttachments = attachments;
        rpInfo.subpassCount = 1;
        rpInfo.pSubpasses = &subpass;
        rpInfo.dependencyCount = 1;
        rpInfo.pDependencies = &dep;

        if (vkCreateRenderPass(device_, &rpInfo, nullptr, &offscreenPass_) != VK_SUCCESS)
            return false;
    }

    // Framebuffer
    VkImageView fbViews[] = {colorView_, depthView_};
    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = offscreenPass_;
    fbInfo.attachmentCount = 2;
    fbInfo.pAttachments = fbViews;
    fbInfo.width = w;
    fbInfo.height = h;
    fbInfo.layers = 1;

    if (vkCreateFramebuffer(device_, &fbInfo, nullptr, &offscreenFB_) != VK_SUCCESS)
        return false;

    // Color sampler (for blit pass)
    if (!colorSampler_) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        if (vkCreateSampler(device_, &samplerInfo, nullptr, &colorSampler_) != VK_SUCCESS)
            return false;
    }

    fbWidth_ = width;
    fbHeight_ = height;
    return true;
}

void VulkanRenderBackend::DestroyOffscreenTarget() {
    if (offscreenFB_) { vkDestroyFramebuffer(device_, offscreenFB_, nullptr); offscreenFB_ = VK_NULL_HANDLE; }
    if (colorView_)   { vkDestroyImageView(device_, colorView_, nullptr); colorView_ = VK_NULL_HANDLE; }
    if (colorImage_)  { vkDestroyImage(device_, colorImage_, nullptr); colorImage_ = VK_NULL_HANDLE; }
    if (colorMemory_) { vkFreeMemory(device_, colorMemory_, nullptr); colorMemory_ = VK_NULL_HANDLE; }
    if (depthView_)   { vkDestroyImageView(device_, depthView_, nullptr); depthView_ = VK_NULL_HANDLE; }
    if (depthImage_)  { vkDestroyImage(device_, depthImage_, nullptr); depthImage_ = VK_NULL_HANDLE; }
    if (depthMemory_) { vkFreeMemory(device_, depthMemory_, nullptr); depthMemory_ = VK_NULL_HANDLE; }
    fbWidth_ = fbHeight_ = 0;
}

// ============================================================================
// Descriptor Layouts
// ============================================================================

bool VulkanRenderBackend::CreateDescriptorLayouts() {
    // Set 0: Scene data
    //   Binding 0: TransformUBO (vertex+fragment) — model/view/proj/cameraPos
    //   Binding 1: LightBuffer SSBO (fragment) — array of Light structs
    //   Binding 2: SceneUBO (fragment) — lightCount, time, etc.
    //   Binding 3: AudioSampleBuffer SSBO (fragment) — audio samples for reactive shaders
    {
        VkDescriptorSetLayoutBinding bindings[4] = {};
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[3].binding = 3;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[3].descriptorCount = 1;
        bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 4;
        layoutInfo.pBindings = bindings;

        if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &sceneLayout_) != VK_SUCCESS)
            return false;
    }

    // Set 1: Material UBO
    {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;

        if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &materialLayout_) != VK_SUCCESS)
            return false;
    }

    // Set 2: Texture samplers (up to 8 textures)
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

        if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &textureLayout_) != VK_SUCCESS)
            return false;
    }

    return true;
}

bool VulkanRenderBackend::CreateDescriptorPool() {
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 64},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 64},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128},
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 256;
    poolInfo.poolSizeCount = 4;
    poolInfo.pPoolSizes = poolSizes;

    return vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_) == VK_SUCCESS;
}

bool VulkanRenderBackend::CreateSceneDescriptors() {
    // Compute aligned size for dynamic UBO offset
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physDevice_, &props);
    VkDeviceSize minAlign = props.limits.minUniformBufferOffsetAlignment;
    VkDeviceSize rawSize = sizeof(TransformUBO);
    transformAlignedSize_ = static_cast<uint32_t>(
        (rawSize + minAlign - 1) & ~(minAlign - 1));

    // Allocate transform UBO with 2 regions (meshes at 0, sky at alignedSize)
    if (!CreateBufferMapped(transformAlignedSize_ * 2,
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            transformUBO_, transformMemory_, &transformMapped_))
        return false;

    // Allocate scene UBO (lightCount etc.)
    if (!CreateBufferMapped(sizeof(SceneUBO),
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            sceneUBO_, sceneUBOMemory_, &sceneUBOMapped_))
        return false;

    // Allocate light SSBO
    size_t lightSSBOSize = sizeof(GPULight) * kMaxLights;
    if (!CreateBufferMapped(lightSSBOSize,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            lightSSBO_, lightSSBOMemory_, &lightSSBOMapped_))
        return false;

    // Allocate audio SSBO (placeholder — 4 bytes minimum for runtime-sized array)
    if (!CreateBuffer(4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      audioSSBO_, audioSSBOMemory_))
        return false;

    // Allocate descriptor set for scene
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &sceneLayout_;

    if (vkAllocateDescriptorSets(device_, &allocInfo, &sceneDescSet_) != VK_SUCCESS)
        return false;

    // Write descriptors: binding 0 = TransformUBO, binding 1 = LightSSBO,
    //                    binding 2 = SceneUBO, binding 3 = AudioSampleBuffer
    VkDescriptorBufferInfo transformBufInfo{};
    transformBufInfo.buffer = transformUBO_;
    transformBufInfo.offset = 0;
    transformBufInfo.range = sizeof(TransformUBO);

    VkDescriptorBufferInfo lightBufInfo{};
    lightBufInfo.buffer = lightSSBO_;
    lightBufInfo.offset = 0;
    lightBufInfo.range = lightSSBOSize;

    VkDescriptorBufferInfo sceneBufInfo{};
    sceneBufInfo.buffer = sceneUBO_;
    sceneBufInfo.offset = 0;
    sceneBufInfo.range = sizeof(SceneUBO);

    VkDescriptorBufferInfo audioBufInfo{};
    audioBufInfo.buffer = audioSSBO_;
    audioBufInfo.offset = 0;
    audioBufInfo.range = 4;

    VkWriteDescriptorSet writes[4] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = sceneDescSet_;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writes[0].pBufferInfo = &transformBufInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = sceneDescSet_;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &lightBufInfo;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = sceneDescSet_;
    writes[2].dstBinding = 2;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[2].pBufferInfo = &sceneBufInfo;

    writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[3].dstSet = sceneDescSet_;
    writes[3].dstBinding = 3;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].pBufferInfo = &audioBufInfo;

    vkUpdateDescriptorSets(device_, 4, writes, 0, nullptr);
    return true;
}

bool VulkanRenderBackend::CreatePipelineLayout() {
    VkDescriptorSetLayout layouts[] = {sceneLayout_, materialLayout_, textureLayout_};

    // Push constant for audio shaders (u_sampleCount: int)
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = 4;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 3;
    layoutInfo.pSetLayouts = layouts;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;

    return vkCreatePipelineLayout(device_, &layoutInfo, nullptr, &pipelineLayout_) == VK_SUCCESS;
}

// ============================================================================
// Shader Module & Pipeline Creation
// ============================================================================

bool VulkanRenderBackend::CreateShaderModules() {
    // Load vertex shader SPIR-V
    const auto& vertSPV = kslRegistry_.GetVertexSPIRV();
    if (vertSPV.empty()) {
        KL_ERR("VulkanRender", "No vertex SPIR-V available");
        return false;
    }

    VkShaderModuleCreateInfo moduleInfo{};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = vertSPV.size() * sizeof(uint32_t);
    moduleInfo.pCode = vertSPV.data();

    if (vkCreateShaderModule(device_, &moduleInfo, nullptr, &vertModule_) != VK_SUCCESS) {
        KL_ERR("VulkanRender", "Failed to create vertex shader module");
        return false;
    }

    return true;
}

bool VulkanRenderBackend::CreateScenePipeline(const std::string& name,
                                                const std::vector<uint32_t>& fragSPV,
                                                bool depthTest, bool depthWrite,
                                                VkPipeline& outPipeline,
                                                VkShaderModule& outModule) {
    // Create fragment shader module
    VkShaderModuleCreateInfo fragModInfo{};
    fragModInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragModInfo.codeSize = fragSPV.size() * sizeof(uint32_t);
    fragModInfo.pCode = fragSPV.data();

    if (vkCreateShaderModule(device_, &fragModInfo, nullptr, &outModule) != VK_SUCCESS)
        return false;

    // Shader stages
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule_;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = outModule;
    stages[1].pName = "main";

    // Vertex input (pos3 + normal3 + uv2 = 8 floats, 32 bytes)
    VkVertexInputBindingDescription bindDesc{};
    bindDesc.binding = 0;
    bindDesc.stride = sizeof(VkVertex);
    bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrDescs[3] = {};
    attrDescs[0].location = 0;
    attrDescs[0].binding = 0;
    attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescs[0].offset = offsetof(VkVertex, px);

    attrDescs[1].location = 1;
    attrDescs[1].binding = 0;
    attrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescs[1].offset = offsetof(VkVertex, nx);

    attrDescs[2].location = 2;
    attrDescs[2].binding = 0;
    attrDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrDescs[2].offset = offsetof(VkVertex, u);

    VkPipelineVertexInputStateCreateInfo vertInputInfo{};
    vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInputInfo.vertexBindingDescriptionCount = 1;
    vertInputInfo.pVertexBindingDescriptions = &bindDesc;
    vertInputInfo.vertexAttributeDescriptionCount = 3;
    vertInputInfo.pVertexAttributeDescriptions = attrDescs;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo iaInfo{};
    iaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iaInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Viewport and scissor (dynamic)
    VkPipelineViewportStateCreateInfo vpInfo{};
    vpInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpInfo.viewportCount = 1;
    vpInfo.scissorCount = 1;

    // Rasterization (reads CVars for wireframe and culling)
    VkPipelineRasterizationStateCreateInfo rastInfo{};
    rastInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rastInfo.polygonMode = cvar_r_wireframe.Get() ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rastInfo.cullMode = cvar_r_culling.Get() ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    rastInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rastInfo.lineWidth = 1.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo msInfo{};
    msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo dsInfo{};
    dsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dsInfo.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
    dsInfo.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
    dsInfo.depthCompareOp = VK_COMPARE_OP_LESS;

    // Color blending
    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                 VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttach.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo cbInfo{};
    cbInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbInfo.attachmentCount = 1;
    cbInfo.pAttachments = &blendAttach;

    // Dynamic state
    VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynInfo{};
    dynInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynInfo.dynamicStateCount = 2;
    dynInfo.pDynamicStates = dynStates;

    VkGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeInfo.stageCount = 2;
    pipeInfo.pStages = stages;
    pipeInfo.pVertexInputState = &vertInputInfo;
    pipeInfo.pInputAssemblyState = &iaInfo;
    pipeInfo.pViewportState = &vpInfo;
    pipeInfo.pRasterizationState = &rastInfo;
    pipeInfo.pMultisampleState = &msInfo;
    pipeInfo.pDepthStencilState = &dsInfo;
    pipeInfo.pColorBlendState = &cbInfo;
    pipeInfo.pDynamicState = &dynInfo;
    pipeInfo.layout = pipelineLayout_;
    pipeInfo.renderPass = offscreenPass_;
    pipeInfo.subpass = 0;

    return vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeInfo,
                                     nullptr, &outPipeline) == VK_SUCCESS;
}

void VulkanRenderBackend::InitShaderPipelines() {
    auto shaderNames = kslRegistry_.ListSPIRVShaders();
    KL_LOG("VulkanRender", "Loading %zu SPIR-V shader pipelines", shaderNames.size());

    // Shaders that use different vertex inputs or descriptor layouts
    static const std::unordered_set<std::string> kSkipShaders = {
        "blit", "line", "ui"
    };

    for (const auto& name : shaderNames) {
        if (kSkipShaders.count(name)) continue;

        const auto& fragSPV = kslRegistry_.GetFragmentSPIRV(name);
        if (fragSPV.empty()) continue;

        PipelineCacheEntry entry;
        bool useDepth = cvar_r_depthtest.Get();
        if (CreateScenePipeline(name, fragSPV, useDepth, useDepth,
                                entry.pipeline, entry.fragModule)) {
            pipelineCache_[name] = entry;
        } else {
            KL_ERR("VulkanRender", "Failed to create pipeline for '%s'",
                   name.c_str());
        }
    }

    KL_LOG("VulkanRender", "Created %zu pipelines", pipelineCache_.size());
}

// ============================================================================
// SPIR-V File Loader
// ============================================================================

VkShaderModule VulkanRenderBackend::LoadSPIRVFile(const char* path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) return VK_NULL_HANDLE;

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> code(fileSize);
    file.seekg(0);
    file.read(code.data(), static_cast<std::streamsize>(fileSize));

    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = fileSize;
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule mod;
    if (vkCreateShaderModule(device_, &info, nullptr, &mod) != VK_SUCCESS)
        return VK_NULL_HANDLE;
    return mod;
}

// ============================================================================
// Blit Pipeline (off-screen -> swapchain)
// ============================================================================

bool VulkanRenderBackend::CreateBlitPipeline() {
    // Load blit SPIR-V shaders
    std::vector<std::string> searchPaths = {"build/shaders/spirv", "../build/shaders/spirv",
                                             "shaders/spirv"};
    for (const auto& dir : searchPaths) {
        std::string vertPath = dir + "/blit.vert.spv";
        std::string fragPath = dir + "/blit.frag.spv";
        blitVertModule_ = LoadSPIRVFile(vertPath.c_str());
        blitFragModule_ = LoadSPIRVFile(fragPath.c_str());
        if (blitVertModule_ && blitFragModule_) break;
        if (blitVertModule_) { vkDestroyShaderModule(device_, blitVertModule_, nullptr); blitVertModule_ = VK_NULL_HANDLE; }
        if (blitFragModule_) { vkDestroyShaderModule(device_, blitFragModule_, nullptr); blitFragModule_ = VK_NULL_HANDLE; }
    }
    if (!blitVertModule_ || !blitFragModule_) {
        KL_ERR("VulkanRender", "Failed to load blit SPIR-V shaders");
        return false;
    }

    // Descriptor set layout: one combined image sampler at binding 0
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;
    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &blitDescLayout_) != VK_SUCCESS)
        return false;

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipeLayoutInfo{};
    pipeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLayoutInfo.setLayoutCount = 1;
    pipeLayoutInfo.pSetLayouts = &blitDescLayout_;
    if (vkCreatePipelineLayout(device_, &pipeLayoutInfo, nullptr, &blitPipelineLayout_) != VK_SUCCESS)
        return false;

    // Allocate per-frame descriptor sets for the off-screen color image
    VkDescriptorSetLayout blitLayouts[kBlitFrames_] = {blitDescLayout_, blitDescLayout_};
    VkDescriptorSetAllocateInfo descAllocInfo{};
    descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descAllocInfo.descriptorPool = descriptorPool_;
    descAllocInfo.descriptorSetCount = kBlitFrames_;
    descAllocInfo.pSetLayouts = blitLayouts;
    if (vkAllocateDescriptorSets(device_, &descAllocInfo, blitDescSets_) != VK_SUCCESS)
        return false;

    // Shader stages
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = blitVertModule_;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = blitFragModule_;
    stages[1].pName = "main";

    // Vertex input: pos2 + uv2 (QuadVertex)
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(QuadVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2] = {};
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[0].offset = 0;  // pos
    attrs[1].location = 1;
    attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[1].offset = 8;  // uv

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAsm{};
    inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAsm.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Dynamic viewport + scissor
    VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynState{};
    dynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynState.dynamicStateCount = 2;
    dynState.pDynamicStates = dynStates;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.lineWidth = 1.0f;
    raster.cullMode = VK_CULL_MODE_NONE;

    VkPipelineMultisampleStateCreateInfo msaa{};
    msaa.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // No depth test for fullscreen blit
    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = VK_FALSE;
    depth.depthWriteEnable = VK_FALSE;

    // No blending - opaque blit
    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttach.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttach;

    VkGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeInfo.stageCount = 2;
    pipeInfo.pStages = stages;
    pipeInfo.pVertexInputState = &vertexInput;
    pipeInfo.pInputAssemblyState = &inputAsm;
    pipeInfo.pViewportState = &viewportState;
    pipeInfo.pRasterizationState = &raster;
    pipeInfo.pMultisampleState = &msaa;
    pipeInfo.pDepthStencilState = &depth;
    pipeInfo.pColorBlendState = &blend;
    pipeInfo.pDynamicState = &dynState;
    pipeInfo.layout = blitPipelineLayout_;
    pipeInfo.renderPass = display_->GetRenderPass();
    pipeInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &blitPipeline_) != VK_SUCCESS) {
        KL_ERR("VulkanRender", "Failed to create blit pipeline");
        return false;
    }

    return true;
}

// ============================================================================
// Quad VBO
// ============================================================================

bool VulkanRenderBackend::CreateQuadVBO() {
    VkDeviceSize size = sizeof(s_blitQuadVerts);
    if (!CreateBuffer(size,
                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      quadVBO_, quadMemory_))
        return false;

    void* mapped;
    if (vkMapMemory(device_, quadMemory_, 0, size, 0, &mapped) != VK_SUCCESS)
        return false;
    memcpy(mapped, s_blitQuadVerts, size);
    vkUnmapMemory(device_, quadMemory_);
    return true;
}

// ============================================================================
// Debug Line Pipeline
// ============================================================================

bool VulkanRenderBackend::CreateLinePipeline() {
    // Load line SPIR-V shaders
    std::vector<std::string> searchPaths = {"build/shaders/spirv", "../build/shaders/spirv",
                                             "shaders/spirv"};
    for (const auto& dir : searchPaths) {
        lineVertModule_ = LoadSPIRVFile((dir + "/line.vert.spv").c_str());
        lineFragModule_ = LoadSPIRVFile((dir + "/line.frag.spv").c_str());
        if (lineVertModule_ && lineFragModule_) break;
        if (lineVertModule_) { vkDestroyShaderModule(device_, lineVertModule_, nullptr); lineVertModule_ = VK_NULL_HANDLE; }
        if (lineFragModule_) { vkDestroyShaderModule(device_, lineFragModule_, nullptr); lineFragModule_ = VK_NULL_HANDLE; }
    }
    if (!lineVertModule_ || !lineFragModule_) return false;

    // Descriptor set layout: one UBO at binding 0 (view+projection matrices)
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboBinding;
    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &lineDescLayout_) != VK_SUCCESS)
        return false;

    VkPipelineLayoutCreateInfo pipeLayoutInfo{};
    pipeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLayoutInfo.setLayoutCount = 1;
    pipeLayoutInfo.pSetLayouts = &lineDescLayout_;
    if (vkCreatePipelineLayout(device_, &pipeLayoutInfo, nullptr, &linePipeLayout_) != VK_SUCCESS)
        return false;

    // Allocate UBO for view+projection (2 mat4 = 128 bytes)
    if (!CreateBufferMapped(128, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            lineUBO_, lineUBOMemory_, &lineUBOMapped_))
        return false;

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &lineDescLayout_;
    if (vkAllocateDescriptorSets(device_, &allocInfo, &lineDescSet_) != VK_SUCCESS)
        return false;

    VkDescriptorBufferInfo bufInfo{};
    bufInfo.buffer = lineUBO_;
    bufInfo.offset = 0;
    bufInfo.range = 128;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = lineDescSet_;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &bufInfo;
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

    // Vertex input: pos3 + color4 = 7 floats = 28 bytes
    VkVertexInputBindingDescription bindDesc{};
    bindDesc.stride = 28;
    bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrDescs[2] = {};
    attrDescs[0].location = 0;
    attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;    // position
    attrDescs[0].offset = 0;
    attrDescs[1].location = 1;
    attrDescs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;  // color
    attrDescs[1].offset = 12;

    VkPipelineVertexInputStateCreateInfo vertInput{};
    vertInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertInput.vertexBindingDescriptionCount = 1;
    vertInput.pVertexBindingDescriptions = &bindDesc;
    vertInput.vertexAttributeDescriptionCount = 2;
    vertInput.pVertexAttributeDescriptions = attrDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAsm{};
    inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAsm.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynState{};
    dynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynState.dynamicStateCount = 2;
    dynState.pDynamicStates = dynStates;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.lineWidth = 1.0f;
    raster.cullMode = VK_CULL_MODE_NONE;

    VkPipelineMultisampleStateCreateInfo msaa{};
    msaa.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Two pipelines: depth-tested and non-depth-tested
    // Create depth-tested version first
    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = VK_TRUE;
    depth.depthWriteEnable = VK_FALSE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttach.blendEnable = VK_TRUE;
    blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttach.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttach.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttach;

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = lineVertModule_;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = lineFragModule_;
    stages[1].pName = "main";

    VkGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeInfo.stageCount = 2;
    pipeInfo.pStages = stages;
    pipeInfo.pVertexInputState = &vertInput;
    pipeInfo.pInputAssemblyState = &inputAsm;
    pipeInfo.pViewportState = &viewportState;
    pipeInfo.pRasterizationState = &raster;
    pipeInfo.pMultisampleState = &msaa;
    pipeInfo.pDepthStencilState = &depth;
    pipeInfo.pColorBlendState = &blend;
    pipeInfo.pDynamicState = &dynState;
    pipeInfo.layout = linePipeLayout_;
    pipeInfo.renderPass = offscreenPass_;
    pipeInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeInfo,
                                   nullptr, &linePipeline_) != VK_SUCCESS)
        return false;

    return true;
}

// ============================================================================
// Sky Resources
// ============================================================================

bool VulkanRenderBackend::CreateSkyResources() {
    VkDeviceSize size = sizeof(s_skyQuadVerts);
    if (!CreateBuffer(size,
                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      skyVBO_, skyMemory_))
        return false;

    void* mapped;
    if (vkMapMemory(device_, skyMemory_, 0, size, 0, &mapped) != VK_SUCCESS)
        return false;
    memcpy(mapped, s_skyQuadVerts, size);
    vkUnmapMemory(device_, skyMemory_);
    return true;
}

// ============================================================================
// Initialize / Shutdown
// ============================================================================

bool VulkanRenderBackend::Initialize() {
    if (initialized_) return true;
    if (!display_ || !display_->IsInitialized()) {
        KL_ERR("VulkanRender", "Display backend not initialized");
        return false;
    }

    device_         = display_->GetDevice();
    physDevice_     = display_->GetPhysicalDevice();
    graphicsQueue_  = display_->GetGraphicsQueue();
    graphicsFamily_ = display_->GetGraphicsFamily();

    // Command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamily_;

    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &cmdPool_) != VK_SUCCESS) {
        KL_ERR("VulkanRender", "Failed to create command pool");
        return false;
    }

    // Command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = cmdPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(device_, &allocInfo, &cmdBuffer_);

    // Scan SPIR-V shaders (with .kso CPU modules for parameter support)
    ksl::KSLSymbolTable symbols;
    symbols.RegisterAll();

    struct ShaderSearchPath { std::string spirv; };
    std::vector<ShaderSearchPath> searchPaths = {
        {"build/shaders/spirv"},
        {"../build/shaders/spirv"},
        {"shaders/spirv"},
    };
    for (const auto& sp : searchPaths) {
        int count = kslRegistry_.ScanSPIRVDirectory(sp.spirv);
        if (count > 0) {
            KL_LOG("VulkanRender", "Loaded %d SPIR-V shaders from %s", count, sp.spirv.c_str());
            // Also load .kso CPU modules from parent directory for material parameter support
            std::string ksoDir = sp.spirv;
            auto pos = ksoDir.rfind("/spirv");
            if (pos != std::string::npos) ksoDir = ksoDir.substr(0, pos);
            int ksoCount = kslRegistry_.ScanDirectory(ksoDir, "", &symbols);
            if (ksoCount > 0)
                KL_LOG("VulkanRender", "Loaded %d KSO CPU modules from %s", ksoCount, ksoDir.c_str());
            break;
        }
    }

    if (!kslRegistry_.HasSPIRV()) {
        KL_ERR("VulkanRender", "No SPIR-V shaders found");
        return false;
    }

    // Make registry available to KSLMaterial for script-based shader binding
    KSLMaterial::SetRegistry(&kslRegistry_);

    // Create descriptor layouts, pool, and scene descriptors
    if (!CreateDescriptorLayouts()) {
        KL_ERR("VulkanRender", "Failed to create descriptor layouts");
        return false;
    }
    if (!CreateDescriptorPool()) {
        KL_ERR("VulkanRender", "Failed to create descriptor pool");
        return false;
    }
    if (!CreatePipelineLayout()) {
        KL_ERR("VulkanRender", "Failed to create pipeline layout");
        return false;
    }
    if (!CreateSceneDescriptors()) {
        KL_ERR("VulkanRender", "Failed to create scene descriptors");
        return false;
    }

    // Create initial off-screen render target (will be resized on first RenderDirect)
    if (!CreateOffscreenTarget(64, 64)) {
        KL_ERR("VulkanRender", "Failed to create off-screen target");
        return false;
    }

    // Create vertex shader module
    if (!CreateShaderModules()) {
        KL_ERR("VulkanRender", "Failed to create shader modules");
        return false;
    }

    // Create scene pipelines from SPIR-V
    InitShaderPipelines();

    // Sky resources
    CreateSkyResources();
    CreateQuadVBO();

    // Blit pipeline (fullscreen quad to composite off-screen -> swapchain)
    if (!CreateBlitPipeline()) {
        KL_WARN("VulkanRender", "Blit pipeline creation failed");
    }

    // Debug line pipeline (grid, axes, debug visualization)
    if (!CreateLinePipeline()) {
        KL_WARN("VulkanRender", "Line pipeline creation failed (debug lines disabled)");
    }

    KL_LOG("VulkanRender", "Initialized successfully (Vulkan)");
    initialized_ = true;
    return true;
}

void VulkanRenderBackend::Shutdown() {
    if (!initialized_) return;
    initialized_ = false;
    if (device_) vkDeviceWaitIdle(device_);

    CleanupMeshCache();
    CleanupTextureCache();
    CleanupMaterialCache();
    CleanupPipelineCache();

    // Destroy sky resources
    DestroyBuffer(skyVBO_, skyMemory_);
    if (skyPipeline_) vkDestroyPipeline(device_, skyPipeline_, nullptr);
    if (skyFragModule_) vkDestroyShaderModule(device_, skyFragModule_, nullptr);
    DestroyBuffer(quadVBO_, quadMemory_);
    DestroyBuffer(batchVBO_, batchMemory_);

    // Destroy line resources
    DestroyBuffer(lineVBO_, lineMemory_);
    DestroyBuffer(lineUBO_, lineUBOMemory_);
    if (linePipeline_) vkDestroyPipeline(device_, linePipeline_, nullptr);
    if (lineVertModule_) vkDestroyShaderModule(device_, lineVertModule_, nullptr);
    if (lineFragModule_) vkDestroyShaderModule(device_, lineFragModule_, nullptr);
    if (linePipeLayout_) vkDestroyPipelineLayout(device_, linePipeLayout_, nullptr);
    if (lineDescLayout_) vkDestroyDescriptorSetLayout(device_, lineDescLayout_, nullptr);

    // Destroy overlay resources
    if (overlayPipeline_) vkDestroyPipeline(device_, overlayPipeline_, nullptr);
    DestroyBuffer(overlayBuffer_, overlayMemory_);
    DestroyBuffer(overlayStagingBuf_, overlayStagingMem_);
    if (overlayImageView_) vkDestroyImageView(device_, overlayImageView_, nullptr);
    if (overlayImage_) vkDestroyImage(device_, overlayImage_, nullptr);
    if (overlayImageMemory_) vkFreeMemory(device_, overlayImageMemory_, nullptr);
    if (overlaySampler_) vkDestroySampler(device_, overlaySampler_, nullptr);

    // Destroy blit resources
    if (blitPipeline_) vkDestroyPipeline(device_, blitPipeline_, nullptr);
    if (blitPipelineLayout_) vkDestroyPipelineLayout(device_, blitPipelineLayout_, nullptr);
    if (blitDescLayout_) vkDestroyDescriptorSetLayout(device_, blitDescLayout_, nullptr);
    if (blitVertModule_) vkDestroyShaderModule(device_, blitVertModule_, nullptr);
    if (blitFragModule_) vkDestroyShaderModule(device_, blitFragModule_, nullptr);

    // Readback
    DestroyBuffer(readbackBuffer_, readbackMemory_);

    // Scene descriptors
    DestroyBuffer(transformUBO_, transformMemory_);
    DestroyBuffer(sceneUBO_, sceneUBOMemory_);
    DestroyBuffer(lightSSBO_, lightSSBOMemory_);
    DestroyBuffer(audioSSBO_, audioSSBOMemory_);

    // Vertex shader module
    if (vertModule_) vkDestroyShaderModule(device_, vertModule_, nullptr);

    // Pink pipeline
    if (pinkPipeline_) vkDestroyPipeline(device_, pinkPipeline_, nullptr);

    // Descriptor pool
    if (descriptorPool_) vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);

    // Pipeline layout
    if (pipelineLayout_) vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);

    // Descriptor layouts
    if (sceneLayout_) vkDestroyDescriptorSetLayout(device_, sceneLayout_, nullptr);
    if (materialLayout_) vkDestroyDescriptorSetLayout(device_, materialLayout_, nullptr);
    if (textureLayout_) vkDestroyDescriptorSetLayout(device_, textureLayout_, nullptr);

    // Off-screen target
    DestroyOffscreenTarget();
    if (offscreenPass_) vkDestroyRenderPass(device_, offscreenPass_, nullptr);
    if (colorSampler_) vkDestroySampler(device_, colorSampler_, nullptr);

    // Command pool
    if (cmdPool_) vkDestroyCommandPool(device_, cmdPool_, nullptr);

    initialized_ = false;
}

bool VulkanRenderBackend::IsInitialized() const {
    return initialized_;
}

const char* VulkanRenderBackend::GetName() const {
    return "Vulkan";
}

// ============================================================================
// Mesh Upload & Caching
// ============================================================================

VulkanRenderBackend::MeshCacheEntry& VulkanRenderBackend::UploadMesh(Mesh* mesh) {
    uintptr_t key = reinterpret_cast<uintptr_t>(mesh);
    uint32_t currentVersion = mesh->GetGPUVersion();
    auto it = meshCache_.find(key);
    if (it != meshCache_.end() && it->second.lastVersion == currentVersion) {
        return it->second;
    }

    ITriangleGroup* triGroup = mesh->GetTriangleGroup();
    uint32_t triCount = triGroup->GetTriangleCount();
    Triangle3D* triangles = triGroup->GetTriangles();

    bool hasUV = mesh->HasUV();
    const Vector2D* uvVerts = hasUV ? mesh->GetUVVertices() : nullptr;
    const IndexGroup* uvIndices = hasUV ? mesh->GetUVIndexGroup() : nullptr;

    // Build interleaved vertex data
    static std::vector<VkVertex> verts;
    verts.clear();
    verts.reserve(triCount * 3);

    for (uint32_t i = 0; i < triCount; ++i) {
        const Triangle3D& tri = triangles[i];
        const Vector3D& p1 = *tri.p1;
        const Vector3D& p2 = *tri.p2;
        const Vector3D& p3 = *tri.p3;

        Vector3D n = (p2 - p1).CrossProduct(p3 - p1);
        float len = n.Magnitude();
        if (len > 1e-8f) n = n * (1.0f / len);

        float u0 = 0, v0 = 0, u1 = 0, v1 = 0, u2 = 0, v2 = 0;
        if (hasUV && uvIndices && uvVerts) {
            const IndexGroup& uvIdx = uvIndices[i];
            u0 = uvVerts[uvIdx.A].X; v0 = uvVerts[uvIdx.A].Y;
            u1 = uvVerts[uvIdx.B].X; v1 = uvVerts[uvIdx.B].Y;
            u2 = uvVerts[uvIdx.C].X; v2 = uvVerts[uvIdx.C].Y;
        }

        verts.push_back({p1.X, p1.Y, p1.Z, n.X, n.Y, n.Z, u0, v0});
        verts.push_back({p2.X, p2.Y, p2.Z, n.X, n.Y, n.Z, u1, v1});
        verts.push_back({p3.X, p3.Y, p3.Z, n.X, n.Y, n.Z, u2, v2});
    }

    VkDeviceSize dataSize = verts.size() * sizeof(VkVertex);

    if (it != meshCache_.end()) {
        MeshCacheEntry& entry = it->second;
        if (static_cast<size_t>(entry.vertexCount) == verts.size()) {
            // Same size: update in-place
            void* mapped;
            vkMapMemory(device_, entry.memory, 0, dataSize, 0, &mapped);
            memcpy(mapped, verts.data(), dataSize);
            vkUnmapMemory(device_, entry.memory);
        } else {
            // Different size: recreate buffer
            DestroyBuffer(entry.buffer, entry.memory);
            CreateBuffer(dataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         entry.buffer, entry.memory);
            void* mapped;
            vkMapMemory(device_, entry.memory, 0, dataSize, 0, &mapped);
            memcpy(mapped, verts.data(), dataSize);
            vkUnmapMemory(device_, entry.memory);
            entry.vertexCount = static_cast<int>(verts.size());
        }
        entry.lastVersion = currentVersion;
        return entry;
    }

    // New mesh
    MeshCacheEntry entry;
    entry.vertexCount = static_cast<int>(verts.size());
    entry.lastVersion = currentVersion;

    CreateBuffer(dataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 entry.buffer, entry.memory);

    void* mapped;
    vkMapMemory(device_, entry.memory, 0, dataSize, 0, &mapped);
    memcpy(mapped, verts.data(), dataSize);
    vkUnmapMemory(device_, entry.memory);

    auto result = meshCache_.emplace(key, entry);
    return result.first->second;
}

void VulkanRenderBackend::CleanupMeshCache() {
    for (auto& [key, entry] : meshCache_) {
        DestroyBuffer(entry.buffer, entry.memory);
    }
    meshCache_.clear();
}

// ============================================================================
// Texture Upload & Caching
// ============================================================================

VulkanRenderBackend::TextureCacheEntry& VulkanRenderBackend::UploadTexture(Texture* tex) {
    uintptr_t key = reinterpret_cast<uintptr_t>(tex);
    auto it = textureCache_.find(key);
    if (it != textureCache_.end()) return it->second;

    uint32_t w = tex->GetWidth();
    uint32_t h = tex->GetHeight();

    // Prepare RGBA data
    std::vector<uint8_t> rgba(w * h * 4);
    if (tex->GetFormat() == Texture::Format::RGB888) {
        const Color888* pixels = tex->GetPixels();
        for (uint32_t i = 0; i < w * h; ++i) {
            rgba[i * 4 + 0] = pixels[i].R;
            rgba[i * 4 + 1] = pixels[i].G;
            rgba[i * 4 + 2] = pixels[i].B;
            rgba[i * 4 + 3] = 255;
        }
    } else {
        // Palette mode
        const uint8_t* indices = tex->GetIndices();
        const uint8_t* palette = tex->GetPalette();
        for (uint32_t i = 0; i < w * h; ++i) {
            uint8_t idx = indices[i];
            rgba[i * 4 + 0] = palette[idx * 3 + 0];
            rgba[i * 4 + 1] = palette[idx * 3 + 1];
            rgba[i * 4 + 2] = palette[idx * 3 + 2];
            rgba[i * 4 + 3] = 255;
        }
    }

    // Create staging buffer
    VkDeviceSize imageSize = w * h * 4;
    VkBuffer stagingBuf;
    VkDeviceMemory stagingMem;
    CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuf, stagingMem);

    void* mapped;
    vkMapMemory(device_, stagingMem, 0, imageSize, 0, &mapped);
    memcpy(mapped, rgba.data(), imageSize);
    vkUnmapMemory(device_, stagingMem);

    // Create image
    TextureCacheEntry entry;
    CreateImage(w, h, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                entry.image, entry.memory);

    entry.view = CreateImageView(entry.image, VK_FORMAT_R8G8B8A8_UNORM,
                                 VK_IMAGE_ASPECT_COLOR_BIT);

    // Transition, copy, transition
    auto cmd = BeginOneShot();
    TransitionImageLayout(cmd, entry.image, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_ASPECT_COLOR_BIT);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {w, h, 1};
    vkCmdCopyBufferToImage(cmd, stagingBuf, entry.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    TransitionImageLayout(cmd, entry.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_IMAGE_ASPECT_COLOR_BIT);
    EndOneShot(cmd);

    // Cleanup staging
    DestroyBuffer(stagingBuf, stagingMem);

    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if (vkCreateSampler(device_, &samplerInfo, nullptr, &entry.sampler) != VK_SUCCESS) {
        KL_ERR("VulkanRender", "Failed to create texture sampler");
        return entry;
    }

    // Allocate and write texture descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &textureLayout_;
    if (vkAllocateDescriptorSets(device_, &allocInfo, &entry.descSet) != VK_SUCCESS) {
        KL_ERR("VulkanRender", "Failed to allocate texture descriptor set");
        return entry;
    }

    VkDescriptorImageInfo imgInfo{};
    imgInfo.sampler = entry.sampler;
    imgInfo.imageView = entry.view;
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = entry.descSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imgInfo;

    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

    auto result = textureCache_.emplace(key, entry);
    return result.first->second;
}

void VulkanRenderBackend::CleanupTextureCache() {
    for (auto& [key, entry] : textureCache_) {
        if (entry.sampler) vkDestroySampler(device_, entry.sampler, nullptr);
        if (entry.view) vkDestroyImageView(device_, entry.view, nullptr);
        if (entry.image) vkDestroyImage(device_, entry.image, nullptr);
        if (entry.memory) vkFreeMemory(device_, entry.memory, nullptr);
    }
    textureCache_.clear();
}

// ============================================================================
// Material UBO Management
// ============================================================================

VulkanRenderBackend::MaterialCacheEntry& VulkanRenderBackend::GetOrCreateMaterial(
    const KSLMaterial* kmat) {
    uintptr_t key = reinterpret_cast<uintptr_t>(kmat);
    auto it = materialCache_.find(key);
    if (it != materialCache_.end()) return it->second;

    MaterialCacheEntry entry;

    // Determine material UBO size from KSL params using std140 layout rules
    ksl::KSLModule* mod = kmat->GetModule();
    if (!mod) {
        materialCache_[key] = entry;
        return materialCache_[key];
    }

    ksl::ParamList params = mod->GetParams();
    size_t uboSize = 0;
    for (int i = 0; i < params.count; ++i) {
        const auto& decl = params.decls[i];
        int elemCount = (decl.flags == ksl::ParamFlags::Array && decl.arraySize > 1)
                        ? decl.arraySize : 1;
        size_t alignment = 4, dataSize = 4, elemStride = 4;
        switch (decl.type) {
            case ksl::ParamType::Float: alignment = 4;  dataSize = 4;  elemStride = 4;  break;
            case ksl::ParamType::Int:
            case ksl::ParamType::Bool:  alignment = 4;  dataSize = 4;  elemStride = 4;  break;
            case ksl::ParamType::Vec2:  alignment = 8;  dataSize = 8;  elemStride = 8;  break;
            case ksl::ParamType::Vec3:  alignment = 16; dataSize = 12; elemStride = 16; break;
            case ksl::ParamType::Vec4:  alignment = 16; dataSize = 16; elemStride = 16; break;
        }
        uboSize = (uboSize + alignment - 1) & ~(alignment - 1);
        if (elemCount > 1)
            uboSize += elemStride * elemCount;  // Array: all elements use stride
        else
            uboSize += dataSize;  // Scalar: actual size, alignment handles padding
    }

    // Minimum 16 bytes for alignment
    if (uboSize < 16) uboSize = 16;
    // Round up to device minUniformBufferOffsetAlignment
    VkPhysicalDeviceProperties devProps;
    vkGetPhysicalDeviceProperties(physDevice_, &devProps);
    size_t align = devProps.limits.minUniformBufferOffsetAlignment;
    if (align > 0) uboSize = (uboSize + align - 1) & ~(align - 1);

    entry.size = uboSize;
    CreateBufferMapped(uboSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       entry.ubo, entry.memory, &entry.mapped);

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &materialLayout_;
    if (vkAllocateDescriptorSets(device_, &allocInfo, &entry.descSet) != VK_SUCCESS) {
        KL_ERR("VulkanRender", "Failed to allocate material descriptor set");
        return entry;
    }

    VkDescriptorBufferInfo bufInfo{};
    bufInfo.buffer = entry.ubo;
    bufInfo.offset = 0;
    bufInfo.range = uboSize;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = entry.descSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    write.pBufferInfo = &bufInfo;
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

    materialCache_[key] = entry;
    return materialCache_[key];
}

void VulkanRenderBackend::UpdateMaterialUBO(MaterialCacheEntry& entry,
                                             const KSLMaterial& kmat) {
    if (!entry.mapped || entry.size == 0) return;

    void* inst = kmat.GetInstance();
    if (!inst) return;

    ksl::KSLModule* mod = kmat.GetModule();
    if (!mod) return;

    // Marshal C++ instance data into std140 UBO layout
    ksl::ParamList params = mod->GetParams();
    auto* dst = static_cast<uint8_t*>(entry.mapped);
    auto* src = static_cast<const uint8_t*>(inst);

    memset(dst, 0, entry.size);

    size_t dstOffset = 0;
    for (int i = 0; i < params.count; ++i) {
        const auto& decl = params.decls[i];
        int elemCount = (decl.flags == ksl::ParamFlags::Array && decl.arraySize > 1)
                        ? decl.arraySize : 1;

        size_t alignment = 4, dataSize = 4, elemStride = 4;
        switch (decl.type) {
            case ksl::ParamType::Float: alignment = 4;  dataSize = 4;  elemStride = 4;  break;
            case ksl::ParamType::Int:
            case ksl::ParamType::Bool:  alignment = 4;  dataSize = 4;  elemStride = 4;  break;
            case ksl::ParamType::Vec2:  alignment = 8;  dataSize = 8;  elemStride = 8;  break;
            case ksl::ParamType::Vec3:  alignment = 16; dataSize = 12; elemStride = 16; break;
            case ksl::ParamType::Vec4:  alignment = 16; dataSize = 16; elemStride = 16; break;
        }

        // Align destination to std140 rules
        dstOffset = (dstOffset + alignment - 1) & ~(alignment - 1);

        // Copy each element from C++ layout to std140 layout
        for (int e = 0; e < elemCount; ++e) {
            size_t srcFieldOffset = decl.offset + dataSize * e;
            if (dstOffset + dataSize <= entry.size) {
                memcpy(dst + dstOffset, src + srcFieldOffset, dataSize);
            }
            if (elemCount > 1) {
                // Array: all elements use uniform stride (std140 rule 4)
                dstOffset += elemStride;
            } else {
                // Scalar: advance by actual data size; next param's alignment
                // handles any required padding (vec3 is 12 bytes, not 16)
                dstOffset += dataSize;
            }
        }
    }
}

void VulkanRenderBackend::CleanupMaterialCache() {
    for (auto& [key, entry] : materialCache_) {
        DestroyBuffer(entry.ubo, entry.memory);
    }
    materialCache_.clear();
}

void VulkanRenderBackend::CleanupPipelineCache() {
    for (auto& [name, entry] : pipelineCache_) {
        if (entry.pipeline) vkDestroyPipeline(device_, entry.pipeline, nullptr);
        if (entry.fragModule) vkDestroyShaderModule(device_, entry.fragModule, nullptr);
    }
    pipelineCache_.clear();
}

// ============================================================================
// Pipeline Selection
// ============================================================================

VkPipeline VulkanRenderBackend::GetPipelineForMaterial(const KSLMaterial* kmat) {
    if (!kmat || !kmat->GetModule()) return pinkPipeline_;

    const std::string& name = kmat->GetModule()->Name();
    auto it = pipelineCache_.find(name);
    if (it != pipelineCache_.end()) return it->second.pipeline;

    // Try creating pipeline on-demand from SPIR-V
    const auto& fragSPV = kslRegistry_.GetFragmentSPIRV(name);
    if (fragSPV.empty()) return pinkPipeline_;

    PipelineCacheEntry entry;
    bool useDepth = cvar_r_depthtest.Get();
    if (CreateScenePipeline(name, fragSPV, useDepth, useDepth,
                            entry.pipeline, entry.fragModule)) {
        pipelineCache_[name] = entry;
        return entry.pipeline;
    }

    return pinkPipeline_;
}

// ============================================================================
// Sky Rendering
// ============================================================================

void VulkanRenderBackend::RenderSky(VkCommandBuffer cmd, CameraBase* camera, int vpW, int vpH) {
    if (!cmd || !camera || !skyVBO_) return;

    Sky& sky = Sky::GetInstance();
    bool hasSky = sky.IsEnabled() || camera->HasSkyGradient();
    if (!hasSky) return;

    KSLMaterial* kmat = sky.GetMaterial();
    if (!kmat || !kmat->IsBound() || !kmat->GetModule()) return;

    // Create sky pipeline on first use (no depth test - sky is background)
    if (!skyPipeline_) {
        const auto& fragSPV = kslRegistry_.GetFragmentSPIRV(kmat->GetModule()->Name());
        if (fragSPV.empty()) return;
        if (!CreateScenePipeline(kmat->GetModule()->Name() + "_sky", fragSPV,
                                 false, false, skyPipeline_, skyFragModule_))
            return;
    }

    // Set identity transform for sky (screen-space quad) - written to second UBO region
    TransformUBO skyTransform{};
    for (int i = 0; i < 4; ++i) {
        skyTransform.model[i * 4 + i] = 1.0f;
        skyTransform.view[i * 4 + i] = 1.0f;
        skyTransform.projection[i * 4 + i] = 1.0f;
    }
    void* skyRegion = static_cast<uint8_t*>(transformMapped_) + transformAlignedSize_;
    memcpy(skyRegion, &skyTransform, sizeof(TransformUBO));

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline_);
    // Bind scene descriptor with dynamic offset pointing to sky's transform region
    uint32_t skyDynOffset = transformAlignedSize_;
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout_, 0, 1, &sceneDescSet_, 1, &skyDynOffset);

    // Bind material descriptor
    auto& matEntry = GetOrCreateMaterial(kmat);
    UpdateMaterialUBO(matEntry, *kmat);

    uint32_t offset = 0;
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout_, 1, 1, &matEntry.descSet, 1, &offset);

    VkDeviceSize vbOffset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &skyVBO_, &vbOffset);
    vkCmdDraw(cmd, 6, 1, 0, 0);
}

// ============================================================================
// Debug Line Rendering
// ============================================================================

void VulkanRenderBackend::RenderDebugLines(const float* viewMat, const float* projMat) {
    auto& dd = DebugDraw::GetInstance();
    if (!dd.IsEnabled()) return;
    const auto& lines = dd.GetLines();
    if (lines.empty() || !linePipeline_ || !lineUBOMapped_) return;

    // Build vertex data: 2 vertices per line, 7 floats each (pos3 + color4)
    std::vector<float> verts;
    verts.reserve(lines.size() * 2 * 7);

    size_t depthTestedCount = 0;
    for (const auto& l : lines) {
        if (!l.depthTest) continue;
        verts.push_back(l.start.X); verts.push_back(l.start.Y); verts.push_back(l.start.Z);
        verts.push_back(l.color.r); verts.push_back(l.color.g); verts.push_back(l.color.b); verts.push_back(l.color.a);
        verts.push_back(l.end.X); verts.push_back(l.end.Y); verts.push_back(l.end.Z);
        verts.push_back(l.color.r); verts.push_back(l.color.g); verts.push_back(l.color.b); verts.push_back(l.color.a);
        depthTestedCount += 2;
    }
    size_t noDepthCount = 0;
    for (const auto& l : lines) {
        if (l.depthTest) continue;
        verts.push_back(l.start.X); verts.push_back(l.start.Y); verts.push_back(l.start.Z);
        verts.push_back(l.color.r); verts.push_back(l.color.g); verts.push_back(l.color.b); verts.push_back(l.color.a);
        verts.push_back(l.end.X); verts.push_back(l.end.Y); verts.push_back(l.end.Z);
        verts.push_back(l.color.r); verts.push_back(l.color.g); verts.push_back(l.color.b); verts.push_back(l.color.a);
        noDepthCount += 2;
    }
    if (verts.empty()) return;

    // Upload UBO (view + projection)
    memcpy(lineUBOMapped_, viewMat, 64);
    memcpy(static_cast<uint8_t*>(lineUBOMapped_) + 64, projMat, 64);

    // Upload vertex data
    VkDeviceSize vbSize = verts.size() * sizeof(float);
    if (vbSize > lineVBOSize_) {
        DestroyBuffer(lineVBO_, lineMemory_);
        if (!CreateBuffer(vbSize,
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          lineVBO_, lineMemory_)) return;
        lineVBOSize_ = vbSize;
    }
    void* mapped;
    if (vkMapMemory(device_, lineMemory_, 0, vbSize, 0, &mapped) != VK_SUCCESS) return;
    memcpy(mapped, verts.data(), vbSize);
    vkUnmapMemory(device_, lineMemory_);

    VkCommandBuffer cmd = display_->BeginFrame();
    if (!cmd) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline_);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            linePipeLayout_, 0, 1, &lineDescSet_, 0, nullptr);

    VkDeviceSize vbOffset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &lineVBO_, &vbOffset);

    // Draw depth-tested lines
    if (depthTestedCount > 0)
        vkCmdDraw(cmd, static_cast<uint32_t>(depthTestedCount), 1, 0, 0);

    // Draw non-depth-tested lines (pipeline has depth test enabled,
    // but these are drawn on top anyway since depth write is off for lines)
    if (noDepthCount > 0)
        vkCmdDraw(cmd, static_cast<uint32_t>(noDepthCount), 1,
                  static_cast<uint32_t>(depthTestedCount), 0);
}

// ============================================================================
// RenderDirect (Main Render Path)
// ============================================================================

void VulkanRenderBackend::RenderDirect(Scene* scene, CameraBase* camera) {
    KL_PERF_SCOPE("GPU.Total");
    if (!initialized_ || !scene || !camera || camera->Is2D()) {
        return;
    }

    // Rebuild scene pipelines if render CVars changed
    {
        bool wire  = cvar_r_wireframe.Get();
        bool cull  = cvar_r_culling.Get();
        bool depth = cvar_r_depthtest.Get();
        if (wire != cachedWireframe_ || cull != cachedCulling_ || depth != cachedDepthTest_) {
            cachedWireframe_ = wire;
            cachedCulling_   = cull;
            cachedDepthTest_ = depth;
            vkDeviceWaitIdle(device_);
            CleanupPipelineCache();
            InitShaderPipelines();
        }
    }

    Vector2D minCoord = camera->GetCameraMinCoordinate();
    Vector2D maxCoord = camera->GetCameraMaxCoordinate();
    int vpW = static_cast<int>(maxCoord.X - minCoord.X + 1);
    int vpH = static_cast<int>(maxCoord.Y - minCoord.Y + 1);
    if (vpW <= 0 || vpH <= 0) return;

    // Resize off-screen target if needed
    if (fbWidth_ != vpW || fbHeight_ != vpH) {
        vkDeviceWaitIdle(device_);
        CreateOffscreenTarget(vpW, vpH);
    }

    // Use the display command buffer (already begun by BeginFrame)
    VkCommandBuffer cmd = display_->BeginFrame();
    if (!cmd) return;

    // Begin off-screen render pass
    VkClearValue clearValues[2] = {};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = offscreenPass_;
    rpInfo.framebuffer = offscreenFB_;
    rpInfo.renderArea.extent = {static_cast<uint32_t>(vpW), static_cast<uint32_t>(vpH)};
    rpInfo.clearValueCount = 2;
    rpInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport and scissor (negative height for OpenGL-compatible Y-up)
    VkViewport viewport{};
    viewport.y = static_cast<float>(vpH);
    viewport.width = static_cast<float>(vpW);
    viewport.height = -static_cast<float>(vpH);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = {static_cast<uint32_t>(vpW), static_cast<uint32_t>(vpH)};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Sky first (no depth test - meshes overdraw on top)
    { KL_PERF_SCOPE("GPU.Sky");
        RenderSky(cmd, camera, vpW, vpH);
    }

    // Compute view/projection matrices
    camera->GetTransform()->SetBaseRotation(camera->GetCameraLayout()->GetRotation());
    Quaternion lookDir = camera->GetTransform()->GetRotation().Multiply(camera->GetLookOffset());
    Vector3D camPos = camera->GetTransform()->GetPosition();
    Vector3D forward = lookDir.RotateVector(Vector3D(0, 0, -1));
    Vector3D up = lookDir.RotateVector(Vector3D(0, 1, 0));
    Matrix4x4 viewMat = Matrix4x4::LookAt(camPos, camPos + forward, up);

    float aspect = static_cast<float>(vpW) / static_cast<float>(vpH);
    Matrix4x4 projMat;
    if (camera->IsPerspective()) {
        float fovRad = camera->GetFOV() * 3.14159265f / 180.0f;
        projMat = Matrix4x4::Perspective(fovRad, aspect,
                                          camera->GetNearPlane(), camera->GetFarPlane());
    } else {
        float halfW = static_cast<float>(vpW) * 0.5f;
        float halfH = static_cast<float>(vpH) * 0.5f;
        projMat = Matrix4x4::Orthographic(-halfW, halfW, -halfH, halfH,
                                           camera->GetNearPlane(), camera->GetFarPlane());
    }

    // Remap OpenGL depth range [-1,1] to Vulkan [0,1]:
    // new_row2 = 0.5 * old_row2 + 0.5 * old_row3
    for (int c = 0; c < 4; ++c)
        projMat.M[2][c] = 0.5f * projMat.M[2][c] + 0.5f * projMat.M[3][c];

    Matrix4x4 viewT = viewMat.Transpose();
    Matrix4x4 projT = projMat.Transpose();

    // Update transform UBO
    TransformUBO transformData{};
    // Identity model matrix
    for (int i = 0; i < 4; ++i) transformData.model[i * 4 + i] = 1.0f;
    memcpy(transformData.view, &viewT.M[0][0], 64);
    memcpy(transformData.projection, &projT.M[0][0], 64);
    transformData.cameraPos[0] = camPos.X;
    transformData.cameraPos[1] = camPos.Y;
    transformData.cameraPos[2] = camPos.Z;
    transformData.cameraPos[3] = 0.0f;
    memcpy(transformMapped_, &transformData, sizeof(TransformUBO));

    // Render meshes
    { KL_PERF_SCOPE("GPU.Meshes");

    unsigned int meshCount = scene->GetMeshCount();
    Mesh** meshes = scene->GetMeshes();

    VkPipeline lastPipeline = VK_NULL_HANDLE;

    for (unsigned int i = 0; i < meshCount; ++i) {
        Mesh* mesh = meshes[i];
        if (!mesh || !mesh->IsEnabled()) continue;
        ITriangleGroup* triGroup = mesh->GetTriangleGroup();
        if (!triGroup || triGroup->GetTriangleCount() == 0) continue;
        IMaterial* material = mesh->GetMaterial();
        if (!material) continue;

        const KSLMaterial* kmat = static_cast<const KSLMaterial*>(
            material->IsKSL() ? material : nullptr);

        VkPipeline pipeline = VK_NULL_HANDLE;
        if (kmat && kmat->IsBound() && kmat->GetModule()) {
            pipeline = GetPipelineForMaterial(kmat);
        }
        if (!pipeline) pipeline = pinkPipeline_;
        if (!pipeline) continue; // No pipeline available

        // Bind pipeline if changed
        if (pipeline != lastPipeline) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            // Bind scene descriptor (set 0) with dynamic offset 0 = mesh transforms
            uint32_t meshDynOffset = 0;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineLayout_, 0, 1, &sceneDescSet_, 1, &meshDynOffset);
            lastPipeline = pipeline;
        }

        // Bind material descriptor (set 1)
        if (kmat && kmat->IsBound()) {
            auto& matEntry = GetOrCreateMaterial(kmat);
            UpdateMaterialUBO(matEntry, *kmat);
            uint32_t offset = 0;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineLayout_, 1, 1, &matEntry.descSet, 1, &offset);

            // Bind textures (set 2)
            if (kmat->TextureCount() > 0) {
                Texture* tex = kmat->GetTexture(0);
                if (tex) {
                    auto& texEntry = UploadTexture(tex);
                    if (texEntry.descSet) {
                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                pipelineLayout_, 2, 1, &texEntry.descSet,
                                                0, nullptr);
                    }
                }
            }

            // Update light SSBO and scene UBO (lightCount)
            const auto& lights = kmat->GetLights();
            int lightCount = static_cast<int>(std::min(lights.size(), static_cast<size_t>(kMaxLights)));

            SceneUBO sceneData{};
            sceneData.lightCount = lightCount;
            memcpy(sceneUBOMapped_, &sceneData, sizeof(SceneUBO));

            GPULight gpuLights[kMaxLights] = {};
            for (int li = 0; li < lightCount; ++li) {
                const auto& ld = lights[li];
                gpuLights[li].position[0] = ld.position.x;
                gpuLights[li].position[1] = ld.position.y;
                gpuLights[li].position[2] = ld.position.z;
                gpuLights[li].intensity = ld.intensity;
                gpuLights[li].color[0] = ld.color.x;
                gpuLights[li].color[1] = ld.color.y;
                gpuLights[li].color[2] = ld.color.z;
                gpuLights[li].falloff = ld.falloff;
                gpuLights[li].curve = ld.curve;
            }
            memcpy(lightSSBOMapped_, gpuLights, sizeof(gpuLights));
        }

        // Upload and draw mesh
        auto& meshEntry = UploadMesh(mesh);
        VkDeviceSize vbOffset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &meshEntry.buffer, &vbOffset);
        vkCmdDraw(cmd, meshEntry.vertexCount, 1, 0, 0);
    }

    } // end GPU.Meshes scope

    // Debug lines (grid, axes, etc.) — drawn in offscreen pass with scene view/projection
    { KL_PERF_SCOPE("GPU.DebugLines");
        RenderDebugLines(&viewT.M[0][0], &projT.M[0][0]);
    }

    // End off-screen render pass
    vkCmdEndRenderPass(cmd);

    // Transition off-screen image to shader-readable for blit
    TransitionImageLayout(cmd, colorImage_,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_IMAGE_ASPECT_COLOR_BIT);
}

// ============================================================================
// Render (with CPU readback)
// ============================================================================

void VulkanRenderBackend::Render(Scene* scene, CameraBase* camera) {
    RenderDirect(scene, camera);
}

// ============================================================================
// ReadPixels
// ============================================================================

void VulkanRenderBackend::ReadPixels(Color888* buffer, int width, int height) {
    if (!initialized_ || !buffer || !colorImage_) return;
    if (width != fbWidth_ || height != fbHeight_) return;

    size_t pixelCount = static_cast<size_t>(width) * height;
    size_t bufSize = pixelCount * 4; // RGBA

    // Create/resize readback staging buffer
    if (readbackSize_ < bufSize) {
        DestroyBuffer(readbackBuffer_, readbackMemory_);
        CreateBufferMapped(bufSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                           readbackBuffer_, readbackMemory_, &readbackMapped_);
        readbackSize_ = bufSize;
    }

    // Copy off-screen image to staging buffer
    auto cmd = BeginOneShot();

    // Transition to transfer src
    TransitionImageLayout(cmd, colorImage_,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          VK_IMAGE_ASPECT_COLOR_BIT);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {static_cast<uint32_t>(width),
                          static_cast<uint32_t>(height), 1};

    vkCmdCopyImageToBuffer(cmd, colorImage_,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           readbackBuffer_, 1, &region);

    // Transition back
    TransitionImageLayout(cmd, colorImage_,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_IMAGE_ASPECT_COLOR_BIT);

    EndOneShot(cmd);

    // Copy to output (RGBA -> RGB, no row flip needed for Vulkan - origin is top-left)
    const uint8_t* src = static_cast<const uint8_t*>(readbackMapped_);
    for (size_t i = 0; i < pixelCount; ++i) {
        buffer[i].R = src[i * 4 + 0];
        buffer[i].G = src[i * 4 + 1];
        buffer[i].B = src[i * 4 + 2];
    }
}

// ============================================================================
// BlitToScreen (off-screen -> swapchain via image blit)
// ============================================================================

void VulkanRenderBackend::BlitToScreen(int screenW, int screenH) {
    if (!initialized_ || !colorImage_ || !blitPipeline_ || !quadVBO_) return;

    // Get the display's active command buffer
    VkCommandBuffer displayCmd = display_->BeginFrame();
    if (!displayCmd) return;

    // Start the swapchain render pass for blit output
    display_->BeginSwapchainRenderPass();

    // Update the blit descriptor set for the current frame (avoids race with in-flight frame)
    uint32_t frameIdx = display_->GetCurrentFrame() % kBlitFrames_;
    VkDescriptorSet curBlitDesc = blitDescSets_[frameIdx];

    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imgInfo.imageView   = colorView_;
    imgInfo.sampler     = colorSampler_;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = curBlitDesc;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imgInfo;
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

    // Set viewport and scissor for the swapchain
    VkViewport viewport{};
    viewport.width  = static_cast<float>(display_->GetSwapchainWidth());
    viewport.height = static_cast<float>(display_->GetSwapchainHeight());
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(displayCmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = {display_->GetSwapchainWidth(), display_->GetSwapchainHeight()};
    vkCmdSetScissor(displayCmd, 0, 1, &scissor);

    // Draw fullscreen quad sampling from off-screen color image
    vkCmdBindPipeline(displayCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blitPipeline_);
    vkCmdBindDescriptorSets(displayCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                             blitPipelineLayout_, 0, 1, &curBlitDesc, 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(displayCmd, 0, 1, &quadVBO_, &offset);
    vkCmdDraw(displayCmd, 6, 1, 0, 0);
}

// ============================================================================
// Canvas Overlay Compositing
// ============================================================================

void VulkanRenderBackend::CompositeCanvasOverlays(int screenW, int screenH) {
    auto& canvases = Canvas2D::ActiveList();
    if (canvases.empty()) return;
    if (!blitPipeline_ || !blitPipelineLayout_) return;

    VkCommandBuffer cmd = display_->BeginFrame();
    if (!cmd || !quadVBO_) return;

    // Create overlay pipeline on first use (same as blit but with alpha blending)
    if (!overlayPipeline_) {
        VkPipelineShaderStageCreateInfo stages[2] = {};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = blitVertModule_;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = blitFragModule_;
        stages[1].pName = "main";

        VkVertexInputBindingDescription binding{};
        binding.stride = sizeof(QuadVertex);
        VkVertexInputAttributeDescription attrs[2] = {};
        attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[1].location = 1; attrs[1].format = VK_FORMAT_R32G32_SFLOAT; attrs[1].offset = 8;

        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = 2;
        vertexInput.pVertexAttributeDescriptions = attrs;

        VkPipelineInputAssemblyStateCreateInfo inputAsm{};
        inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAsm.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynState{};
        dynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynState.dynamicStateCount = 2;
        dynState.pDynamicStates = dynStates;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo raster{};
        raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.lineWidth = 1.0f;
        raster.cullMode = VK_CULL_MODE_NONE;

        VkPipelineMultisampleStateCreateInfo msaa{};
        msaa.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depth{};
        depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        // Alpha blending: srcAlpha, 1-srcAlpha
        VkPipelineColorBlendAttachmentState blendAttach{};
        blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttach.blendEnable = VK_TRUE;
        blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttach.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttach.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo blend{};
        blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blend.attachmentCount = 1;
        blend.pAttachments = &blendAttach;

        VkGraphicsPipelineCreateInfo pipeInfo{};
        pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeInfo.stageCount = 2;
        pipeInfo.pStages = stages;
        pipeInfo.pVertexInputState = &vertexInput;
        pipeInfo.pInputAssemblyState = &inputAsm;
        pipeInfo.pViewportState = &viewportState;
        pipeInfo.pRasterizationState = &raster;
        pipeInfo.pMultisampleState = &msaa;
        pipeInfo.pDepthStencilState = &depth;
        pipeInfo.pColorBlendState = &blend;
        pipeInfo.pDynamicState = &dynState;
        pipeInfo.layout = blitPipelineLayout_;
        pipeInfo.renderPass = display_->GetRenderPass();
        pipeInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeInfo,
                                       nullptr, &overlayPipeline_) != VK_SUCCESS) {
            KL_ERR("VulkanRender", "Failed to create overlay pipeline");
            return;
        }
    }

    for (auto* canvas : canvases) {
        int cw = canvas->GetWidth();
        int ch = canvas->GetHeight();
        if (cw <= 0 || ch <= 0 || !canvas->IsDirty()) continue;

        const Color888* pixels = canvas->GetPixels();
        const uint8_t* alpha = canvas->GetAlpha();
        size_t totalPixels = static_cast<size_t>(cw) * ch;

        // Convert to RGBA
        if (overlayRgba_.size() != totalPixels * 4)
            overlayRgba_.assign(totalPixels * 4, 0);

        int dx0 = canvas->GetDirtyMinX();
        int dy0 = canvas->GetDirtyMinY();
        int dx1 = canvas->GetDirtyMaxX();
        int dy1 = canvas->GetDirtyMaxY();

        for (int y = dy0; y <= dy1; ++y) {
            int rowOff = y * cw;
            for (int x = dx0; x <= dx1; ++x) {
                int si = rowOff + x;
                int di = si * 4;
                overlayRgba_[di + 0] = pixels[si].R;
                overlayRgba_[di + 1] = pixels[si].G;
                overlayRgba_[di + 2] = pixels[si].B;
                overlayRgba_[di + 3] = alpha[si];
            }
        }

        // Create/resize overlay image if needed
        bool needsRecreate = (overlayImage_ == VK_NULL_HANDLE) ||
                             (overlayTexW_ != cw || overlayTexH_ != ch);
        if (needsRecreate) {
            vkDeviceWaitIdle(device_);
            if (overlayImageView_) vkDestroyImageView(device_, overlayImageView_, nullptr);
            if (overlayImage_) vkDestroyImage(device_, overlayImage_, nullptr);
            if (overlayImageMemory_) vkFreeMemory(device_, overlayImageMemory_, nullptr);

            if (!CreateImage(cw, ch, VK_FORMAT_R8G8B8A8_UNORM,
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                             overlayImage_, overlayImageMemory_))
                return;
            overlayImageView_ = CreateImageView(overlayImage_, VK_FORMAT_R8G8B8A8_UNORM,
                                                 VK_IMAGE_ASPECT_COLOR_BIT);
            if (!overlayImageView_) return;
            overlayTexW_ = cw;
            overlayTexH_ = ch;

            // Create sampler if needed
            if (!overlaySampler_) {
                VkSamplerCreateInfo si{};
                si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                si.magFilter = VK_FILTER_NEAREST;
                si.minFilter = VK_FILTER_NEAREST;
                si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                if (vkCreateSampler(device_, &si, nullptr, &overlaySampler_) != VK_SUCCESS)
                    return;
            }

            // Allocate descriptor set (reuse blit layout)
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptorPool_;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &blitDescLayout_;
            if (vkAllocateDescriptorSets(device_, &allocInfo, &overlayDescSet_) != VK_SUCCESS)
                return;

            // Transition to SHADER_READ initially (will transition to TRANSFER_DST for upload)
            VkCommandBuffer oneshot = BeginOneShot();
            TransitionImageLayout(oneshot, overlayImage_,
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  VK_IMAGE_ASPECT_COLOR_BIT);
            EndOneShot(oneshot);
        }

        // Resize persistent staging buffer if needed
        VkDeviceSize uploadSize = totalPixels * 4;
        if (uploadSize > overlayStagingSize_) {
            DestroyBuffer(overlayStagingBuf_, overlayStagingMem_);
            if (!CreateBuffer(uploadSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              overlayStagingBuf_, overlayStagingMem_)) {
                overlayStagingSize_ = 0;
                return;
            }
            overlayStagingSize_ = uploadSize;
        }

        void* mapped;
        if (vkMapMemory(device_, overlayStagingMem_, 0, uploadSize, 0, &mapped) != VK_SUCCESS)
            return;
        memcpy(mapped, overlayRgba_.data(), uploadSize);
        vkUnmapMemory(device_, overlayStagingMem_);

        VkCommandBuffer upload = BeginOneShot();
        TransitionImageLayout(upload, overlayImage_,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_ASPECT_COLOR_BIT);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {static_cast<uint32_t>(cw), static_cast<uint32_t>(ch), 1};
        vkCmdCopyBufferToImage(upload, overlayStagingBuf_, overlayImage_,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        TransitionImageLayout(upload, overlayImage_,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_IMAGE_ASPECT_COLOR_BIT);
        EndOneShot(upload);

        // Update descriptor
        VkDescriptorImageInfo imgInfo{};
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgInfo.imageView = overlayImageView_;
        imgInfo.sampler = overlaySampler_;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = overlayDescSet_;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imgInfo;
        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

        // Draw fullscreen quad with alpha blending
        VkViewport viewport{};
        viewport.width = static_cast<float>(display_->GetSwapchainWidth());
        viewport.height = static_cast<float>(display_->GetSwapchainHeight());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = {display_->GetSwapchainWidth(), display_->GetSwapchainHeight()};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, overlayPipeline_);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 blitPipelineLayout_, 0, 1, &overlayDescSet_, 0, nullptr);

        VkDeviceSize vbOffset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &quadVBO_, &vbOffset);
        vkCmdDraw(cmd, 6, 1, 0, 0);

        // Clear dirty pixels in persistent buffer
        for (int y = dy0; y <= dy1; ++y)
            std::memset(&overlayRgba_[(y * cw + dx0) * 4], 0, (dx1 - dx0 + 1) * 4);

        canvas->Clear();
    }
}

// ============================================================================
// Frame Management
// ============================================================================

void VulkanRenderBackend::PrepareFrame() {
    // Per-frame resource preparation (descriptor set rotation etc.)
    // Currently no-op - single-buffered descriptors for simplicity.
}

void VulkanRenderBackend::FinishFrame() {
    // Post-render finalization. Currently no-op.
}

} // namespace koilo
