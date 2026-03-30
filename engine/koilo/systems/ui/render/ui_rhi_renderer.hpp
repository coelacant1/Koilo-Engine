// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_rhi_renderer.hpp
 * @brief Unified UI renderer using IRHIDevice abstraction.
 *
 * Replaces separate UIGLRenderer and UIVkRenderer with a single
 * implementation that works with any RHI backend.  Same UIVertex
 * format and batched quad submission strategy.
 *
 * @date 03/27/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/ui/render/draw_list.hpp>
#include <koilo/systems/ui/render/ui_renderer.hpp>
#include <koilo/systems/ui/render/ui_vertex.hpp>
#include <koilo/systems/render/rhi/rhi_types.hpp>
#include <koilo/systems/font/font.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace koilo::rhi { class IRHIDevice; }

namespace koilo {
namespace ui {

/// Unified UI renderer using IRHIDevice.
///
/// Consumes a UIDrawList and renders batched quads through the RHI
/// abstraction layer.  Works identically on OpenGL and Vulkan.
class UIRHIRenderer : public IUIRenderer {
public:
    static constexpr size_t MAX_VERTICES = 65536;
    static constexpr size_t MAX_QUADS    = MAX_VERTICES / 6;

    UIRHIRenderer() = default;
    ~UIRHIRenderer() override { Shutdown(); }

    UIRHIRenderer(const UIRHIRenderer&) = delete;
    UIRHIRenderer& operator=(const UIRHIRenderer&) = delete;

    /// Initialize RHI resources (shaders, pipeline, VBO, textures).
    /// @param device  RHI device (OpenGL or Vulkan).
    /// @param isVulkan  true if the device is a Vulkan backend.
    bool Initialize(rhi::IRHIDevice* device, bool isVulkan);

    // -- IUIRenderer interface ------------------------------------------

    void Shutdown() override;
    bool IsInitialized() const override { return initialized_; }
    bool IsSoftware() const override { return false; }

    uint32_t SetFont(font::Font* font) override;
    uint32_t SetBoldFont(font::Font* font) override;
    void SyncFontAtlases(font::Font* font, uint32_t& fontHandle,
                         font::Font* boldFont, uint32_t& boldHandle) override;
    void Render(const UIDrawList& drawList,
                int viewportW, int viewportH) override;

    // -- RHI-specific methods -------------------------------------------

    /// Upload or re-upload a font glyph atlas texture.
    /// @param atlas   The glyph atlas data.
    /// @param existing  Optional existing texture handle to update (0 = create new).
    /// @return Opaque texture handle (RHITexture.id).
    uint32_t UploadFontAtlas(font::GlyphAtlas& atlas, uint32_t existing = 0);

private:
    rhi::IRHIDevice* device_ = nullptr;
    bool initialized_ = false;
    bool topLeftOrigin_ = false;  // Vulkan/SW: top-left; OpenGL: bottom-left
    bool deferredDraw_  = false;  // Vulkan: true (offset VBO batching); OpenGL/SW: false

    rhi::RHIShader   vertShader_  = {};
    rhi::RHIShader   fragShader_  = {};
    rhi::RHIPipeline pipeline_    = {};
    rhi::RHIBuffer   vbo_         = {};
    rhi::RHITexture  whiteTexture_     = {};
    rhi::RHITexture  fontAtlasTexture_ = {};
    rhi::RHITexture  boldAtlasTexture_ = {};

    UIVertex vertices_[MAX_VERTICES];
    size_t   vertexCount_       = 0;
    size_t   flushOffset_       = 0;
    rhi::RHITexture currentTexture_ = {};
    bool currentUseTexture_     = false;

    struct ScissorState { int x, y, w, h; };
    std::vector<ScissorState> scissorStack_;
    int viewportW_ = 0, viewportH_ = 0;

    void SetTexture(rhi::RHITexture tex, bool isTextured);
    void PushQuad(float x, float y, float w, float h,
                  float u0, float v0, float u1, float v1, Color4 c);
    void PushRoundedQuad(float x, float y, float w, float h,
                         const float radii[4], float borderWidth, Color4 c);
    void EmitBorder(float x, float y, float w, float h,
                    float bw, Color4 c);
    void PushTriangle(float x0, float y0, float x1, float y1,
                      float x2, float y2, Color4 c);
    void Flush();
};

} // namespace ui
} // namespace koilo
