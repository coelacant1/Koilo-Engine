// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_vk_renderer.cpp
 * @brief Vulkan batched UI renderer implementation.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */

#include <koilo/systems/ui/render/ui_vk_renderer.hpp>
#include <koilo/systems/display/backends/gpu/vulkanbackend.hpp>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

namespace koilo {
namespace ui {

// ====================================================================
// Public methods
// ====================================================================

bool UIVkRenderer::Initialize(VulkanBackend* backend) {
    if (initialized_) return true;
    if (!backend || !backend->GetDevice()) return false;
    backend_ = backend;

    if (!CreateDescriptorResources()) return false;
    if (!CreatePipeline()) return false;
    if (!CreateVertexBuffer()) return false;
    if (!CreateWhiteTexture()) return false;

    initialized_ = true;
    return true;
}

void UIVkRenderer::Shutdown() {
    if (!initialized_ || !backend_) return;
    VkDevice dev = backend_->GetDevice();
    vkDeviceWaitIdle(dev);

    DestroyTextureImage(fontImage_, fontMemory_, fontView_);
    DestroyTextureImage(whiteImage_, whiteMemory_, whiteView_);

    for (int i = 0; i < kMaxFrames; ++i) {
        if (vertexBuffer_[i]) {
            vkUnmapMemory(dev, vertexMemory_[i]);
            vkDestroyBuffer(dev, vertexBuffer_[i], nullptr);
            vkFreeMemory(dev, vertexMemory_[i], nullptr);
            vertexBuffer_[i] = VK_NULL_HANDLE;
            vertexMapped_[i] = nullptr;
        }
    }

    if (pipeline_)       { vkDestroyPipeline(dev, pipeline_, nullptr); pipeline_ = VK_NULL_HANDLE; }
    if (pipelineLayout_) { vkDestroyPipelineLayout(dev, pipelineLayout_, nullptr); pipelineLayout_ = VK_NULL_HANDLE; }
    if (vertModule_)     { vkDestroyShaderModule(dev, vertModule_, nullptr); vertModule_ = VK_NULL_HANDLE; }
    if (fragModule_)     { vkDestroyShaderModule(dev, fragModule_, nullptr); fragModule_ = VK_NULL_HANDLE; }
    if (sampler_)        { vkDestroySampler(dev, sampler_, nullptr); sampler_ = VK_NULL_HANDLE; }
    if (descriptorPool_) { vkDestroyDescriptorPool(dev, descriptorPool_, nullptr); descriptorPool_ = VK_NULL_HANDLE; }
    if (descriptorSetLayout_) { vkDestroyDescriptorSetLayout(dev, descriptorSetLayout_, nullptr); descriptorSetLayout_ = VK_NULL_HANDLE; }

    whiteDescSet_ = VK_NULL_HANDLE;
    fontDescSet_ = VK_NULL_HANDLE;
    fontAtlasHandle_ = 0;
    fontAtlasW_ = fontAtlasH_ = 0;
    initialized_ = false;
}

uint32_t UIVkRenderer::UploadFontAtlas(font::GlyphAtlas& atlas) {
    if (!initialized_) return 0;
    VkDevice dev = backend_->GetDevice();

    bool needsRecreate = (fontImage_ == VK_NULL_HANDLE) ||
                         (atlas.Width() != fontAtlasW_) ||
                         (atlas.Height() != fontAtlasH_);

    if (needsRecreate) {
        vkDeviceWaitIdle(dev);
        if (fontDescSet_ != VK_NULL_HANDLE) {
            fontDescSet_ = VK_NULL_HANDLE;
        }
        DestroyTextureImage(fontImage_, fontMemory_, fontView_);

        if (!CreateTextureImage(atlas.Pixels(), atlas.Width(), atlas.Height(),
                                VK_FORMAT_R8_UNORM,
                                fontImage_, fontMemory_, fontView_)) {
            return 0;
        }
        fontAtlasW_ = atlas.Width();
        fontAtlasH_ = atlas.Height();
        fontDescSet_ = AllocateDescriptorSet(fontView_);
        fontAtlasHandle_ = 1;
    } else if (atlas.IsDirty()) {
        // Re-upload pixels to existing image via staging buffer
        VkDeviceSize imageSize = static_cast<VkDeviceSize>(atlas.Width()) * atlas.Height();

        VkBuffer staging;
        VkDeviceMemory stagingMem;
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size = imageSize;
        bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(dev, &bufInfo, nullptr, &staging);

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(dev, staging, &memReq);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkAllocateMemory(dev, &allocInfo, nullptr, &stagingMem);
        vkBindBufferMemory(dev, staging, stagingMem, 0);

        void* mapped;
        vkMapMemory(dev, stagingMem, 0, imageSize, 0, &mapped);
        std::memcpy(mapped, atlas.Pixels(), imageSize);
        vkUnmapMemory(dev, stagingMem);

        VkCommandBuffer cmd = BeginSingleTimeCommands();

        // Transition to TRANSFER_DST
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = fontImage_;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {static_cast<uint32_t>(atlas.Width()),
                              static_cast<uint32_t>(atlas.Height()), 1};
        vkCmdCopyBufferToImage(cmd, staging, fontImage_,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // Transition back to SHADER_READ
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);

        EndSingleTimeCommands(cmd);

        vkDestroyBuffer(dev, staging, nullptr);
        vkFreeMemory(dev, stagingMem, nullptr);
    }

    atlas.ClearDirty();
    return fontAtlasHandle_;
}

void UIVkRenderer::Render(const UIDrawList& drawList, int viewportW, int viewportH,
                           VkCommandBuffer cmd) {
    if (!initialized_ || drawList.Size() == 0 || cmd == VK_NULL_HANDLE) return;

    scissorStack_.clear();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    // Set full viewport
    VkViewport vp{};
    vp.x = 0; vp.y = 0;
    vp.width = static_cast<float>(viewportW);
    vp.height = static_cast<float>(viewportH);
    vp.minDepth = 0.0f; vp.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &vp);

    // Default scissor = full viewport
    VkRect2D defaultScissor{};
    defaultScissor.offset = {0, 0};
    defaultScissor.extent = {static_cast<uint32_t>(viewportW),
                             static_cast<uint32_t>(viewportH)};
    vkCmdSetScissor(cmd, 0, 1, &defaultScissor);

    // Select per-frame vertex buffer to avoid GPU/CPU race
    activeFrame_ = static_cast<int>(backend_->GetCurrentFrame()) % kMaxFrames;

    // Bind vertex buffer
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer_[activeFrame_], &offset);

    PushConstants pc;
    pc.viewportW = static_cast<float>(viewportW);
    pc.viewportH = static_cast<float>(viewportH);
    pc.useTexture = 0;

    vertexCount_ = 0;
    flushOffset_ = 0;
    currentDescSet_ = whiteDescSet_;
    bool useTexture = false;

    // Bind white texture initially
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout_, 0, 1, &whiteDescSet_, 0, nullptr);

    // Store render context for overflow flushes in PushQuad/PushTriangle
    renderCmd_ = cmd;
    renderPc_ = pc;
    renderUseTexture_ = useTexture;

    for (size_t i = 0; i < drawList.Size(); ++i) {
        const DrawCmd& dcmd = drawList[i];

        switch (dcmd.type) {
        case DrawCmdType::SolidRect:
            SetTexture(whiteDescSet_, false, useTexture, cmd, pc);
            PushQuad(dcmd.x, dcmd.y, dcmd.w, dcmd.h,
                     0.0f, 0.0f, 1.0f, 1.0f, dcmd.color);
            break;

        case DrawCmdType::BorderRect:
            SetTexture(whiteDescSet_, false, useTexture, cmd, pc);
            EmitBorder(dcmd.x, dcmd.y, dcmd.w, dcmd.h,
                       dcmd.borderWidth, dcmd.color);
            break;

        case DrawCmdType::TexturedRect: {
            bool hasTex = (dcmd.textureHandle != 0) && (fontDescSet_ != VK_NULL_HANDLE);
            VkDescriptorSet ds = hasTex ? fontDescSet_ : whiteDescSet_;
            SetTexture(ds, hasTex, useTexture, cmd, pc);
            PushQuad(dcmd.x, dcmd.y, dcmd.w, dcmd.h,
                     dcmd.u0, dcmd.v0, dcmd.u1, dcmd.v1, dcmd.color);
            break;
        }

        case DrawCmdType::RoundedRect:
            SetTexture(whiteDescSet_, false, useTexture, cmd, pc);
            PushRoundedQuad(dcmd.x, dcmd.y, dcmd.w, dcmd.h,
                            dcmd.cornerRadius, 0.0f, dcmd.color);
            break;

        case DrawCmdType::RoundedBorderRect:
            SetTexture(whiteDescSet_, false, useTexture, cmd, pc);
            PushRoundedQuad(dcmd.x, dcmd.y, dcmd.w, dcmd.h,
                            dcmd.cornerRadius, dcmd.borderWidth, dcmd.color);
            break;

        case DrawCmdType::PushScissor: {
            Flush(useTexture, cmd, pc);
            int sx = dcmd.scissorX;
            int sy = dcmd.scissorY;  // Vulkan Y is top-down like screen space
            int sw = dcmd.scissorW;
            int sh = dcmd.scissorH;
            if (!scissorStack_.empty()) {
                auto& cur = scissorStack_.back();
                int x1 = std::max(sx, cur[0]);
                int y1 = std::max(sy, cur[1]);
                int x2 = std::min(sx + sw, cur[0] + cur[2]);
                int y2 = std::min(sy + sh, cur[1] + cur[3]);
                sx = x1; sy = y1;
                sw = std::max(0, x2 - x1);
                sh = std::max(0, y2 - y1);
            }
            scissorStack_.push_back({sx, sy, sw, sh});
            VkRect2D scissor{};
            scissor.offset = {sx, sy};
            scissor.extent = {static_cast<uint32_t>(sw), static_cast<uint32_t>(sh)};
            vkCmdSetScissor(cmd, 0, 1, &scissor);
            break;
        }

        case DrawCmdType::PopScissor:
            Flush(useTexture, cmd, pc);
            if (!scissorStack_.empty()) scissorStack_.pop_back();
            if (!scissorStack_.empty()) {
                auto& prev = scissorStack_.back();
                VkRect2D scissor{};
                scissor.offset = {prev[0], prev[1]};
                scissor.extent = {static_cast<uint32_t>(prev[2]),
                                  static_cast<uint32_t>(prev[3])};
                vkCmdSetScissor(cmd, 0, 1, &scissor);
            } else {
                vkCmdSetScissor(cmd, 0, 1, &defaultScissor);
            }
            break;

        case DrawCmdType::Line: {
            SetTexture(whiteDescSet_, false, useTexture, cmd, pc);
            float dx = dcmd.w - dcmd.x;
            float dy = dcmd.h - dcmd.y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len < 0.001f) break;
            float hw = dcmd.borderWidth * 0.5f;
            float nx = -dy / len * hw;
            float ny =  dx / len * hw;
            PushTriangle(dcmd.x + nx, dcmd.y + ny,
                         dcmd.x - nx, dcmd.y - ny,
                         dcmd.w + nx, dcmd.h + ny, dcmd.color);
            PushTriangle(dcmd.x - nx, dcmd.y - ny,
                         dcmd.w - nx, dcmd.h - ny,
                         dcmd.w + nx, dcmd.h + ny, dcmd.color);
            break;
        }

        case DrawCmdType::FilledCircle: {
            SetTexture(whiteDescSet_, false, useTexture, cmd, pc);
            float r = dcmd.w;
            float side = r * 2.0f;
            float radii[4] = {r, r, r, r};
            PushRoundedQuad(dcmd.x - r, dcmd.y - r, side, side, radii, 0.0f, dcmd.color);
            break;
        }

        case DrawCmdType::CircleOutline: {
            SetTexture(whiteDescSet_, false, useTexture, cmd, pc);
            float r = dcmd.w;
            float side = r * 2.0f;
            float radii[4] = {r, r, r, r};
            PushRoundedQuad(dcmd.x - r, dcmd.y - r, side, side, radii, dcmd.borderWidth, dcmd.color);
            break;
        }

        case DrawCmdType::Triangle:
            SetTexture(whiteDescSet_, false, useTexture, cmd, pc);
            PushTriangle(dcmd.x, dcmd.y, dcmd.w, dcmd.h, dcmd.x2, dcmd.y2, dcmd.color);
            break;
        }
    }

    Flush(useTexture, cmd, pc);
}

// ====================================================================
// Private batching helpers
// ====================================================================

void UIVkRenderer::SetTexture(VkDescriptorSet descSet, bool isTextured,
                               bool& useTexture, VkCommandBuffer cmd,
                               const PushConstants& pc) {
    if (descSet != currentDescSet_ || isTextured != useTexture) {
        if (vertexCount_ > 0) Flush(useTexture, cmd, pc);
        currentDescSet_ = descSet;
        useTexture = isTextured;
        renderUseTexture_ = isTextured;
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout_, 0, 1, &currentDescSet_, 0, nullptr);
    }
}

void UIVkRenderer::PushQuad(float x, float y, float w, float h,
                             float u0, float v0, float u1, float v1,
                             Color4 c) {
    if (flushOffset_ + vertexCount_ + 6 > MAX_VERTICES) {
        if (vertexCount_ > 0 && renderCmd_ != VK_NULL_HANDLE)
            Flush(renderUseTexture_, renderCmd_, renderPc_);
        if (flushOffset_ + 6 > MAX_VERTICES) return;
    }

    UIVertex* v = &vertices_[vertexCount_];

    v[0] = { x,     y,     u0, v0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };
    v[1] = { x + w, y,     u1, v0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };
    v[2] = { x + w, y + h, u1, v1, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };

    v[3] = { x,     y,     u0, v0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };
    v[4] = { x + w, y + h, u1, v1, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };
    v[5] = { x,     y + h, u0, v1, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };

    vertexCount_ += 6;
}

void UIVkRenderer::PushRoundedQuad(float x, float y, float w, float h,
                                    const float radii[4], float borderWidth,
                                    Color4 c) {
    if (flushOffset_ + vertexCount_ + 6 > MAX_VERTICES) {
        if (vertexCount_ > 0 && renderCmd_ != VK_NULL_HANDLE)
            Flush(renderUseTexture_, renderCmd_, renderPc_);
        if (flushOffset_ + 6 > MAX_VERTICES) return;
    }

    UIVertex* v = &vertices_[vertexCount_];
    float halfW = w * 0.5f;
    float halfH = h * 0.5f;
    float sdf[4] = { halfW, halfH, borderWidth, 0.0f };
    float rv[4] = { radii[0], radii[1], radii[2], radii[3] };

    v[0] = { x,     y,     0.0f, 0.0f, c.r, c.g, c.b, c.a, {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]} };
    v[1] = { x + w, y,     1.0f, 0.0f, c.r, c.g, c.b, c.a, {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]} };
    v[2] = { x + w, y + h, 1.0f, 1.0f, c.r, c.g, c.b, c.a, {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]} };

    v[3] = { x,     y,     0.0f, 0.0f, c.r, c.g, c.b, c.a, {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]} };
    v[4] = { x + w, y + h, 1.0f, 1.0f, c.r, c.g, c.b, c.a, {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]} };
    v[5] = { x,     y + h, 0.0f, 1.0f, c.r, c.g, c.b, c.a, {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]} };

    vertexCount_ += 6;
}

void UIVkRenderer::EmitBorder(float x, float y, float w, float h,
                               float bw, Color4 c) {
    PushQuad(x, y, w, bw, 0, 0, 1, 1, c);
    PushQuad(x, y + h - bw, w, bw, 0, 0, 1, 1, c);
    PushQuad(x, y + bw, bw, h - bw * 2.0f, 0, 0, 1, 1, c);
    PushQuad(x + w - bw, y + bw, bw, h - bw * 2.0f, 0, 0, 1, 1, c);
}

void UIVkRenderer::PushTriangle(float x0, float y0, float x1, float y1,
                                 float ax2, float ay2, Color4 c) {
    if (flushOffset_ + vertexCount_ + 3 > MAX_VERTICES) {
        if (vertexCount_ > 0 && renderCmd_ != VK_NULL_HANDLE)
            Flush(renderUseTexture_, renderCmd_, renderPc_);
        if (flushOffset_ + 3 > MAX_VERTICES) return;
    }
    UIVertex* v = &vertices_[vertexCount_];
    v[0] = { x0, y0, 0, 0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };
    v[1] = { x1, y1, 0, 0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };
    v[2] = { ax2, ay2, 0, 0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };
    vertexCount_ += 3;
}

void UIVkRenderer::Flush(bool useTexture, VkCommandBuffer cmd,
                          const PushConstants& pc) {
    if (vertexCount_ == 0) return;
    if (cmd == VK_NULL_HANDLE) { vertexCount_ = 0; return; }

    // Upload vertices to the next region of the active frame's mapped buffer
    auto* dst = static_cast<uint8_t*>(vertexMapped_[activeFrame_]) + flushOffset_ * sizeof(UIVertex);
    std::memcpy(dst, vertices_, vertexCount_ * sizeof(UIVertex));

    // Update push constants with current useTexture state
    PushConstants pushData = pc;
    pushData.useTexture = useTexture ? 1 : 0;
    vkCmdPushConstants(cmd, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(PushConstants), &pushData);

    vkCmdDraw(cmd, static_cast<uint32_t>(vertexCount_), 1,
              static_cast<uint32_t>(flushOffset_), 0);

    flushOffset_ += vertexCount_;
    vertexCount_ = 0;
}

// ====================================================================
// Resource creation
// ====================================================================

bool UIVkRenderer::CreatePipeline() {
    VkDevice dev = backend_->GetDevice();

    // Load SPIR-V shader modules
    vertModule_ = LoadSPIRV("build/shaders/spirv/ui.vert.spv");
    if (!vertModule_) {
        std::cerr << "[UIVkRenderer] Failed to load ui.vert.spv\n";
        return false;
    }
    fragModule_ = LoadSPIRV("build/shaders/spirv/ui.frag.spv");
    if (!fragModule_) {
        std::cerr << "[UIVkRenderer] Failed to load ui.frag.spv\n";
        return false;
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule_;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule_;
    stages[1].pName = "main";

    // Vertex input - matches UIVertex layout (52 bytes)
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(UIVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[5]{};
    // location 0: position (vec2 float) - offset 0
    attrs[0].location = 0; attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[0].offset = 0;
    // location 1: uv (vec2 float) - offset 8
    attrs[1].location = 1; attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[1].offset = 8;
    // location 2: color (vec4 unorm8) - offset 16
    attrs[2].location = 2; attrs[2].binding = 0;
    attrs[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    attrs[2].offset = 16;
    // location 3: sdf (vec4 float) - offset 20
    attrs[3].location = 3; attrs[3].binding = 0;
    attrs[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[3].offset = 20;
    // location 4: radii (vec4 float) - offset 36
    attrs[4].location = 4; attrs[4].binding = 0;
    attrs[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[4].offset = 36;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 5;
    vertexInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

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
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Alpha blending (SRC_ALPHA, ONE_MINUS_SRC_ALPHA)
    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.blendEnable = VK_TRUE;
    blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttach.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttach.alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &blendAttach;

    // No depth test for UI
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;

    // Push constants: viewport (vec2) + useTexture (int)
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout_;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;
    if (vkCreatePipelineLayout(dev, &layoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        std::cerr << "[UIVkRenderer] Failed to create pipeline layout\n";
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pDynamicState = &dynState;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = backend_->GetRenderPass();
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_) != VK_SUCCESS) {
        std::cerr << "[UIVkRenderer] Failed to create graphics pipeline\n";
        return false;
    }

    return true;
}

VkShaderModule UIVkRenderer::LoadSPIRV(const char* path) {
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
    if (vkCreateShaderModule(backend_->GetDevice(), &info, nullptr, &mod) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return mod;
}

bool UIVkRenderer::CreateWhiteTexture() {
    uint8_t white[4] = {255, 255, 255, 255};
    if (!CreateTextureImage(white, 1, 1, VK_FORMAT_R8G8B8A8_UNORM,
                            whiteImage_, whiteMemory_, whiteView_)) {
        return false;
    }
    whiteDescSet_ = AllocateDescriptorSet(whiteView_);
    return whiteDescSet_ != VK_NULL_HANDLE;
}

bool UIVkRenderer::CreateVertexBuffer() {
    VkDevice dev = backend_->GetDevice();
    VkDeviceSize bufSize = MAX_VERTICES * sizeof(UIVertex);

    for (int i = 0; i < kMaxFrames; ++i) {
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size = bufSize;
        bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(dev, &bufInfo, nullptr, &vertexBuffer_[i]) != VK_SUCCESS)
            return false;

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(dev, vertexBuffer_[i], &memReq);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(dev, &allocInfo, nullptr, &vertexMemory_[i]) != VK_SUCCESS)
            return false;

        vkBindBufferMemory(dev, vertexBuffer_[i], vertexMemory_[i], 0);
        vkMapMemory(dev, vertexMemory_[i], 0, bufSize, 0, &vertexMapped_[i]);
    }
    return true;
}

bool UIVkRenderer::CreateDescriptorResources() {
    VkDevice dev = backend_->GetDevice();

    // Descriptor set layout: one combined image sampler
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;
    if (vkCreateDescriptorSetLayout(dev, &layoutInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS)
        return false;

    // Descriptor pool: up to 4 sets (white, font, + spares)
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 4;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 4;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    if (vkCreateDescriptorPool(dev, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS)
        return false;

    // Sampler: nearest filtering, clamp to edge
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (vkCreateSampler(dev, &samplerInfo, nullptr, &sampler_) != VK_SUCCESS)
        return false;

    return true;
}

VkDescriptorSet UIVkRenderer::AllocateDescriptorSet(VkImageView imageView) {
    VkDevice dev = backend_->GetDevice();

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout_;

    VkDescriptorSet ds;
    if (vkAllocateDescriptorSets(dev, &allocInfo, &ds) != VK_SUCCESS)
        return VK_NULL_HANDLE;

    VkDescriptorImageInfo imgInfo{};
    imgInfo.sampler = sampler_;
    imgInfo.imageView = imageView;
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = ds;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imgInfo;

    vkUpdateDescriptorSets(dev, 1, &write, 0, nullptr);
    return ds;
}

bool UIVkRenderer::CreateTextureImage(const uint8_t* pixels, int w, int h,
                                       VkFormat format,
                                       VkImage& outImage, VkDeviceMemory& outMemory,
                                       VkImageView& outView) {
    VkDevice dev = backend_->GetDevice();
    uint32_t pixelSize = (format == VK_FORMAT_R8_UNORM) ? 1 : 4;
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(w) * h * pixelSize;

    // Create staging buffer
    VkBuffer staging;
    VkDeviceMemory stagingMem;
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size = imageSize;
    bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(dev, &bufInfo, nullptr, &staging);

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(dev, staging, &memReq);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(dev, &allocInfo, nullptr, &stagingMem);
    vkBindBufferMemory(dev, staging, stagingMem, 0);

    void* mapped;
    vkMapMemory(dev, stagingMem, 0, imageSize, 0, &mapped);
    std::memcpy(mapped, pixels, imageSize);
    vkUnmapMemory(dev, stagingMem);

    // Create image
    VkImageCreateInfo imgInfo{};
    imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.format = format;
    imgInfo.extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};
    imgInfo.mipLevels = 1;
    imgInfo.arrayLayers = 1;
    imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(dev, &imgInfo, nullptr, &outImage) != VK_SUCCESS) {
        vkDestroyBuffer(dev, staging, nullptr);
        vkFreeMemory(dev, stagingMem, nullptr);
        return false;
    }

    vkGetImageMemoryRequirements(dev, outImage, &memReq);
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(dev, &allocInfo, nullptr, &outMemory) != VK_SUCCESS) {
        vkDestroyImage(dev, outImage, nullptr);
        vkDestroyBuffer(dev, staging, nullptr);
        vkFreeMemory(dev, stagingMem, nullptr);
        outImage = VK_NULL_HANDLE;
        return false;
    }
    vkBindImageMemory(dev, outImage, outMemory, 0);

    // Copy staging -> image
    VkCommandBuffer cmd = BeginSingleTimeCommands();

    // Transition UNDEFINED -> TRANSFER_DST
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = outImage;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};
    vkCmdCopyBufferToImage(cmd, staging, outImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition TRANSFER_DST -> SHADER_READ
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommands(cmd);

    vkDestroyBuffer(dev, staging, nullptr);
    vkFreeMemory(dev, stagingMem, nullptr);

    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = outImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    // R8 font atlas: identity swizzle - shader reads .r directly
    viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    if (vkCreateImageView(dev, &viewInfo, nullptr, &outView) != VK_SUCCESS) {
        outView = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

void UIVkRenderer::DestroyTextureImage(VkImage& image, VkDeviceMemory& memory,
                                        VkImageView& view) {
    if (!backend_) return;
    VkDevice dev = backend_->GetDevice();
    if (view)   { vkDestroyImageView(dev, view, nullptr); view = VK_NULL_HANDLE; }
    if (image)  { vkDestroyImage(dev, image, nullptr); image = VK_NULL_HANDLE; }
    if (memory) { vkFreeMemory(dev, memory, nullptr); memory = VK_NULL_HANDLE; }
}

uint32_t UIVkRenderer::FindMemoryType(uint32_t typeFilter,
                                       VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(backend_->GetPhysicalDevice(), &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    return 0;
}

VkCommandBuffer UIVkRenderer::BeginSingleTimeCommands() {
    VkDevice dev = backend_->GetDevice();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = backend_->GetGraphicsFamily();

    VkCommandPool tempPool;
    vkCreateCommandPool(dev, &poolInfo, nullptr, &tempPool);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = tempPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(dev, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Store pool handle in a way we can retrieve it - use a simple static
    // since single-time commands are serialized
    singleTimeCmdPool_ = tempPool;

    return cmd;
}

void UIVkRenderer::EndSingleTimeCommands(VkCommandBuffer cmd) {
    VkDevice dev = backend_->GetDevice();
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(backend_->GetGraphicsQueue(), 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(backend_->GetGraphicsQueue());

    vkFreeCommandBuffers(dev, singleTimeCmdPool_, 1, &cmd);
    vkDestroyCommandPool(dev, singleTimeCmdPool_, nullptr);
    singleTimeCmdPool_ = VK_NULL_HANDLE;
}

} // namespace ui
} // namespace koilo
