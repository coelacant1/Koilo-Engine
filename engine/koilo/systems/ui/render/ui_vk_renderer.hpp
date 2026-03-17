// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_vk_renderer.hpp
 * @brief Vulkan batched UI renderer.
 *
 * Vulkan equivalent of UIGLRenderer - consumes a UIDrawList and
 * renders batched quads using a single pipeline with push constants.
 * Supports solid rects, textured rects (font atlas), SDF rounded
 * rects/circles, lines, triangles, and scissor clipping.
 *
 * Requires an active VulkanBackend with a valid device.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/ui/render/draw_list.hpp>
#include <koilo/systems/ui/render/ui_vertex.hpp>
#include <koilo/systems/font/font.hpp>
#include "../../../registry/reflect_macros.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace koilo {

class VulkanBackend;

namespace ui {

/**
 * @class UIVkRenderer
 * @brief Vulkan batched UI renderer.
 *
 * Mirrors UIGLRenderer's interface but uses Vulkan pipelines,
 * descriptor sets, and push constants for rendering.
 */
class UIVkRenderer {
public:
    static constexpr size_t MAX_VERTICES = 65536;
    static constexpr size_t MAX_QUADS = MAX_VERTICES / 6;

    UIVkRenderer() = default;
    ~UIVkRenderer() { Shutdown(); }

    UIVkRenderer(const UIVkRenderer&) = delete;
    UIVkRenderer& operator=(const UIVkRenderer&) = delete;

    /// Initialize Vulkan resources. Call once after Vulkan device is ready.
    bool Initialize(VulkanBackend* backend);

    /// Release Vulkan resources.
    void Shutdown();

    /// Upload a font atlas to GPU. Returns an opaque handle (1-based index).
    uint32_t UploadFontAtlas(font::GlyphAtlas& atlas);

    /// Render a draw list into the currently active command buffer.
    /// Must be called within an active render pass (between BeginFrame/EndFrame).
    void Render(const UIDrawList& drawList, int viewportW, int viewportH,
                VkCommandBuffer cmd);

    bool IsInitialized() const { return initialized_; }
    uint32_t FontAtlasTexture() const { return fontAtlasHandle_; }

private:
    bool initialized_ = false;
    VulkanBackend* backend_ = nullptr;

    // Pipeline
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkShaderModule vertModule_ = VK_NULL_HANDLE;
    VkShaderModule fragModule_ = VK_NULL_HANDLE;

    // Descriptor set for texture sampling
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet whiteDescSet_ = VK_NULL_HANDLE;
    VkDescriptorSet fontDescSet_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;

    // Vertex buffers (per-frame to avoid GPU/CPU race conditions)
    static constexpr int kMaxFrames = 2;
    VkBuffer vertexBuffer_[kMaxFrames] = {};
    VkDeviceMemory vertexMemory_[kMaxFrames] = {};
    void* vertexMapped_[kMaxFrames] = {};
    int activeFrame_ = 0;

    // 1×1 white texture
    VkImage whiteImage_ = VK_NULL_HANDLE;
    VkDeviceMemory whiteMemory_ = VK_NULL_HANDLE;
    VkImageView whiteView_ = VK_NULL_HANDLE;

    // Font atlas texture
    VkImage fontImage_ = VK_NULL_HANDLE;
    VkDeviceMemory fontMemory_ = VK_NULL_HANDLE;
    VkImageView fontView_ = VK_NULL_HANDLE;
    int fontAtlasW_ = 0, fontAtlasH_ = 0;
    uint32_t fontAtlasHandle_ = 0;

    // CPU vertex staging
    UIVertex vertices_[MAX_VERTICES];
    size_t vertexCount_ = 0;
    size_t flushOffset_ = 0; // running offset into GPU vertex buffer
    VkDescriptorSet currentDescSet_ = VK_NULL_HANDLE;

    // Push constant data
    struct PushConstants {
        float viewportW;
        float viewportH;
        int32_t useTexture;

        KL_BEGIN_FIELDS(PushConstants)
            KL_FIELD(PushConstants, viewportW, "Viewport w", __FLT_MIN__, __FLT_MAX__),
            KL_FIELD(PushConstants, viewportH, "Viewport h", __FLT_MIN__, __FLT_MAX__),
            KL_FIELD(PushConstants, useTexture, "Use texture", -2147483648, 2147483647)
        KL_END_FIELDS

        KL_BEGIN_METHODS(PushConstants)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(PushConstants)
            /* No reflected ctors. */
        KL_END_DESCRIBE(PushConstants)

    };

    // Batching helpers
    void SetTexture(VkDescriptorSet descSet, bool isTextured, bool& useTexture,
                    VkCommandBuffer cmd, const PushConstants& pc);
    void PushQuad(float x, float y, float w, float h,
                  float u0, float v0, float u1, float v1, Color4 c);
    void PushRoundedQuad(float x, float y, float w, float h,
                         const float radii[4], float borderWidth, Color4 c);
    void EmitBorder(float x, float y, float w, float h,
                    float bw, Color4 c);
    void PushTriangle(float x0, float y0, float x1, float y1,
                      float x2, float y2, Color4 c);
    void Flush(bool useTexture, VkCommandBuffer cmd, const PushConstants& pc);

    // Resource creation helpers
    bool CreatePipeline();
    VkShaderModule LoadSPIRV(const char* path);
    bool CreateWhiteTexture();
    bool CreateVertexBuffer();
    bool CreateDescriptorResources();
    VkDescriptorSet AllocateDescriptorSet(VkImageView imageView);
    bool CreateTextureImage(const uint8_t* pixels, int w, int h, VkFormat format,
                            VkImage& outImage, VkDeviceMemory& outMemory,
                            VkImageView& outView);
    void DestroyTextureImage(VkImage& image, VkDeviceMemory& memory, VkImageView& view);
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props);
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer cmd);

    std::vector<std::array<int32_t, 4>> scissorStack_;

    // Render-time state for overflow flushes in PushQuad/PushTriangle
    VkCommandBuffer renderCmd_ = VK_NULL_HANDLE;
    bool renderUseTexture_ = false;
    PushConstants renderPc_{};

    // Temporary command pool for single-time commands (texture uploads)
    VkCommandPool singleTimeCmdPool_ = VK_NULL_HANDLE;

    #ifdef KL_HAVE_VULKAN_BACKEND
    KL_BEGIN_FIELDS(UIVkRenderer)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(UIVkRenderer)
        KL_METHOD_AUTO(UIVkRenderer, IsInitialized, "Is initialized"),
        KL_METHOD_AUTO(UIVkRenderer, FontAtlasTexture, "Font atlas texture")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(UIVkRenderer)
        KL_CTOR0(UIVkRenderer)
    KL_END_DESCRIBE(UIVkRenderer)
    #endif

};

} // namespace ui
} // namespace koilo
