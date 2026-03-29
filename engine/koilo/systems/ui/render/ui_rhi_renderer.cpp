// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_rhi_renderer.cpp
 * @brief Unified UI renderer using IRHIDevice -- implementation.
 *
 * Single implementation that replaces UIGLRenderer and UIVkRenderer.
 * Uses IRHIDevice for all GPU calls, working identically on OpenGL
 * and Vulkan backends.
 *
 * @date 03/27/2026
 * @author Coela Can't
 */

#include "ui_rhi_renderer.hpp"
#include <koilo/systems/render/rhi/rhi_device.hpp>
#include <koilo/kernel/logging/log.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <vector>

namespace koilo {
namespace ui {

// -- Embedded GLSL shaders for the OpenGL RHI path -------------------
// Push constants are emulated via a std140 uniform block at binding 7.

static const char* s_uiVertGLSL = R"GLSL(
#version 330 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;
layout(location = 3) in vec4 a_sdf;
layout(location = 4) in vec4 a_radii;

uniform vec2 u_viewport;

out vec2 v_uv;
out vec4 v_color;
out vec4 v_sdf;
out vec4 v_radii;

void main() {
    vec2 ndc = (a_position / u_viewport) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
    v_uv    = a_uv;
    v_color = a_color;
    v_sdf   = a_sdf;
    v_radii = a_radii;
}
)GLSL";

static const char* s_uiFragGLSL = R"GLSL(
#version 330 core
in vec2 v_uv;
in vec4 v_color;
in vec4 v_sdf;
in vec4 v_radii;

uniform int u_useTexture;
uniform sampler2D u_texture;

out vec4 FragColor;

float roundedRectSDF(vec2 p, vec2 b, vec4 r) {
    float cr = (p.x > 0.0)
        ? ((p.y > 0.0) ? r.z : r.y)
        : ((p.y > 0.0) ? r.w : r.x);
    vec2 q = abs(p) - b + vec2(cr);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - cr;
}

void main() {
    float halfW   = v_sdf.x;
    float halfH   = v_sdf.y;
    float borderW = v_sdf.z;
    bool hasRadius = (v_radii.x > 0.0 || v_radii.y > 0.0 ||
                      v_radii.z > 0.0 || v_radii.w > 0.0);

    if (halfW > 0.0 && hasRadius) {
        vec2 halfSize = vec2(halfW, halfH);
        vec2 p = v_uv * halfSize * 2.0 - halfSize;
        float outerD     = roundedRectSDF(p, halfSize, v_radii);
        float outerAlpha = 1.0 - smoothstep(-0.75, 0.75, outerD);

        if (borderW > 0.0) {
            vec4 innerR    = max(v_radii - vec4(borderW), vec4(0.0));
            vec2 innerHalf = halfSize - vec2(borderW);
            float innerD     = roundedRectSDF(p, innerHalf, innerR);
            float innerAlpha = 1.0 - smoothstep(-0.75, 0.75, innerD);
            FragColor = vec4(v_color.rgb, v_color.a * (outerAlpha - innerAlpha));
        } else {
            FragColor = vec4(v_color.rgb, v_color.a * outerAlpha);
        }
    } else if (u_useTexture != 0) {
        float alpha = texture(u_texture, v_uv).a;
        FragColor = vec4(v_color.rgb, v_color.a * alpha);
    } else {
        FragColor = v_color;
    }
}
)GLSL";

// -- Helper: load SPIR-V binary from disk ----------------------------

static std::vector<uint8_t> LoadSPIRVFile(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) return {};
    auto sz = f.tellg();
    if (sz <= 0) return {};
    std::vector<uint8_t> buf(static_cast<size_t>(sz));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(buf.data()), sz);
    return buf;
}

// -- Push constant data layout (std140-compatible, 16 bytes) ---------

struct UIPushConstants {
    float viewportW;
    float viewportH;
    int32_t useTexture;
    int32_t _pad = 0;
};
static_assert(sizeof(UIPushConstants) == 16, "UIPushConstants must be 16 bytes");

// ====================================================================
// Public API
// ====================================================================

bool UIRHIRenderer::Initialize(rhi::IRHIDevice* device, bool isVulkan) {
    if (initialized_) return true;
    if (!device) return false;

    device_    = device;
    isVulkan_  = isVulkan;

    // -- Shaders -----------------------------------------------------
    if (isVulkan) {
        auto vertSPV = LoadSPIRVFile("build/shaders/spirv/ui.vert.spv");
        auto fragSPV = LoadSPIRVFile("build/shaders/spirv/ui.frag.spv");
        if (vertSPV.empty() || fragSPV.empty()) {
            KL_ERR("UIRHIRenderer", "Failed to load UI SPIR-V shaders");
            return false;
        }
        vertShader_ = device_->CreateShader(rhi::RHIShaderStage::Vertex,
                                             vertSPV.data(), vertSPV.size());
        fragShader_ = device_->CreateShader(rhi::RHIShaderStage::Fragment,
                                             fragSPV.data(), fragSPV.size());
    } else {
        vertShader_ = device_->CreateShader(rhi::RHIShaderStage::Vertex,
                                             s_uiVertGLSL,
                                             std::strlen(s_uiVertGLSL));
        fragShader_ = device_->CreateShader(rhi::RHIShaderStage::Fragment,
                                             s_uiFragGLSL,
                                             std::strlen(s_uiFragGLSL));
    }

    if (!vertShader_.IsValid() || !fragShader_.IsValid()) {
        KL_ERR("UIRHIRenderer", "Failed to compile UI shaders");
        Shutdown();
        return false;
    }

    // -- Pipeline ----------------------------------------------------
    rhi::RHIRenderPass swapPass = device_->GetSwapchainRenderPass();

    rhi::RHIPipelineDesc desc{};
    desc.vertexShader    = vertShader_;
    desc.fragmentShader  = fragShader_;
    desc.renderPass      = swapPass;
    desc.topology        = rhi::RHITopology::TriangleList;

    // UIVertex layout: 52 bytes
    desc.vertexStride    = sizeof(UIVertex);
    desc.vertexAttrCount = 5;
    desc.vertexAttrs[0]  = {0, rhi::RHIFormat::RG32F,   0};   // position (8B)
    desc.vertexAttrs[1]  = {1, rhi::RHIFormat::RG32F,   8};   // uv       (8B)
    desc.vertexAttrs[2]  = {2, rhi::RHIFormat::RGBA8_Unorm, 16}; // color (4B)
    desc.vertexAttrs[3]  = {3, rhi::RHIFormat::RGBA32F, 20};  // sdf      (16B)
    desc.vertexAttrs[4]  = {4, rhi::RHIFormat::RGBA32F, 36};  // radii    (16B)

    desc.rasterizer  = {rhi::RHICullMode::None,
                        rhi::RHIFrontFace::CounterClockwise,
                        rhi::RHIPolygonMode::Fill, false};
    desc.depthStencil = {false, false, rhi::RHICompareOp::Always, false};
    desc.blend.enabled  = true;
    desc.blend.srcColor = rhi::RHIBlendFactor::SrcAlpha;
    desc.blend.dstColor = rhi::RHIBlendFactor::OneMinusSrcAlpha;
    desc.blend.colorOp  = rhi::RHIBlendOp::Add;
    desc.blend.srcAlpha = rhi::RHIBlendFactor::SrcAlpha;
    desc.blend.dstAlpha = rhi::RHIBlendFactor::OneMinusSrcAlpha;
    desc.blend.alphaOp  = rhi::RHIBlendOp::Add;
    desc.layoutHint     = rhi::RHIPipelineDesc::LayoutHint::Blit;
    desc.debugName       = "__ui__";

    pipeline_ = device_->CreatePipeline(desc);
    if (!pipeline_.IsValid()) {
        KL_ERR("UIRHIRenderer", "Failed to create UI pipeline");
        Shutdown();
        return false;
    }

    // -- Dynamic vertex buffer ---------------------------------------
    {
        rhi::RHIBufferDesc bd{};
        bd.size        = MAX_VERTICES * sizeof(UIVertex);
        bd.usage       = rhi::RHIBufferUsage::Vertex;
        bd.hostVisible = true;
        vbo_ = device_->CreateBuffer(bd);
    }

    // -- 1x1 white texture for solid fills ---------------------------
    {
        rhi::RHITextureDesc td{};
        td.width  = 1;
        td.height = 1;
        td.format = rhi::RHIFormat::RGBA8_Unorm;
        td.usage  = rhi::RHITextureUsage::Sampled | rhi::RHITextureUsage::TransferDst;
        td.filter = rhi::RHISamplerFilter::Nearest;
        whiteTexture_ = device_->CreateTexture(td);
        uint8_t white[] = {255, 255, 255, 255};
        device_->UpdateTexture(whiteTexture_, white, 4, 1, 1);
    }

    currentTexture_ = whiteTexture_;
    initialized_ = true;
    KL_LOG("UIRHIRenderer", "Initialized (%s)", isVulkan ? "Vulkan" : "OpenGL");
    return true;
}

void UIRHIRenderer::Shutdown() {
    if (!device_) return;
    if (pipeline_.IsValid())         device_->DestroyPipeline(pipeline_);
    if (vertShader_.IsValid())       device_->DestroyShader(vertShader_);
    if (fragShader_.IsValid())       device_->DestroyShader(fragShader_);
    if (vbo_.IsValid())              device_->DestroyBuffer(vbo_);
    if (whiteTexture_.IsValid())     device_->DestroyTexture(whiteTexture_);
    if (fontAtlasTexture_.IsValid()) device_->DestroyTexture(fontAtlasTexture_);
    if (boldAtlasTexture_.IsValid()) device_->DestroyTexture(boldAtlasTexture_);
    pipeline_ = {}; vertShader_ = {}; fragShader_ = {};
    vbo_ = {}; whiteTexture_ = {}; fontAtlasTexture_ = {}; boldAtlasTexture_ = {};
    initialized_ = false;
}

// -- IUIRenderer interface -------------------------------------------

uint32_t UIRHIRenderer::SetFont(font::Font* font) {
    if (!font || !initialized_) return 0;
    uint32_t h = UploadFontAtlas(font->Atlas());
    fontAtlasTexture_.id = h;
    return h;
}

uint32_t UIRHIRenderer::SetBoldFont(font::Font* font) {
    if (!font || !initialized_) return 0;
    uint32_t h = UploadFontAtlas(font->Atlas());
    boldAtlasTexture_.id = h;
    return h;
}

void UIRHIRenderer::SyncFontAtlases(font::Font* font, uint32_t& fontHandle,
                                     font::Font* boldFont, uint32_t& boldHandle) {
    if (font && fontHandle != 0 && font->Atlas().IsDirty()) {
        fontHandle = UploadFontAtlas(font->Atlas(), fontHandle);
        fontAtlasTexture_.id = fontHandle;
    }
    if (boldFont && boldHandle != 0 && boldFont->Atlas().IsDirty()) {
        boldHandle = UploadFontAtlas(boldFont->Atlas(), boldHandle);
        boldAtlasTexture_.id = boldHandle;
    }
}

// -- Font atlas ------------------------------------------------------

uint32_t UIRHIRenderer::UploadFontAtlas(font::GlyphAtlas& atlas, uint32_t existing) {
    if (!initialized_) return 0;

    rhi::RHITexture tex{};
    tex.id = existing;

    if (!tex.IsValid()) {
        // First upload - create a new texture
        rhi::RHITextureDesc td{};
        td.width  = static_cast<uint32_t>(atlas.Width());
        td.height = static_cast<uint32_t>(atlas.Height());
        td.format = rhi::RHIFormat::R8_Unorm;
        td.usage  = rhi::RHITextureUsage::Sampled | rhi::RHITextureUsage::TransferDst;
        td.filter = rhi::RHISamplerFilter::Nearest;
        tex = device_->CreateTexture(td);
        device_->UpdateTexture(tex, atlas.Pixels(),
                               static_cast<size_t>(atlas.Width()) * atlas.Height(),
                               static_cast<uint32_t>(atlas.Width()),
                               static_cast<uint32_t>(atlas.Height()));
    } else if (atlas.IsDirty()) {
        // Atlas data changed - destroy and recreate to handle size changes
        device_->DestroyTexture(tex);
        rhi::RHITextureDesc td{};
        td.width  = static_cast<uint32_t>(atlas.Width());
        td.height = static_cast<uint32_t>(atlas.Height());
        td.format = rhi::RHIFormat::R8_Unorm;
        td.usage  = rhi::RHITextureUsage::Sampled | rhi::RHITextureUsage::TransferDst;
        td.filter = rhi::RHISamplerFilter::Nearest;
        tex = device_->CreateTexture(td);
        device_->UpdateTexture(tex, atlas.Pixels(),
                               static_cast<size_t>(atlas.Width()) * atlas.Height(),
                               static_cast<uint32_t>(atlas.Width()),
                               static_cast<uint32_t>(atlas.Height()));
    }

    atlas.ClearDirty();
    return tex.id;
}

// -- Render ----------------------------------------------------------

void UIRHIRenderer::Render(const UIDrawList& drawList,
                            int viewportW, int viewportH) {
    if (!initialized_ || drawList.Size() == 0) return;

    viewportW_ = viewportW;
    viewportH_ = viewportH;
    scissorStack_.clear();
    vertexCount_ = 0;
    flushOffset_ = 0;
    currentTexture_     = whiteTexture_;
    currentUseTexture_  = false;

    device_->BindPipeline(pipeline_);

    // Set viewport and full-screen scissor
    rhi::RHIViewport vp{};
    vp.x = 0; vp.y = 0;
    vp.width  = static_cast<float>(viewportW);
    vp.height = static_cast<float>(viewportH);
    vp.minDepth = 0.0f; vp.maxDepth = 1.0f;
    device_->SetViewport(vp);

    rhi::RHIScissor sc{};
    sc.x = 0; sc.y = 0;
    sc.width  = static_cast<uint32_t>(viewportW);
    sc.height = static_cast<uint32_t>(viewportH);
    device_->SetScissor(sc);

    for (size_t i = 0; i < drawList.Size(); ++i) {
        const DrawCmd& cmd = drawList[i];

        switch (cmd.type) {
        case DrawCmdType::SolidRect:
            SetTexture(whiteTexture_, false);
            PushQuad(cmd.x, cmd.y, cmd.w, cmd.h,
                     0.0f, 0.0f, 1.0f, 1.0f, cmd.color);
            break;

        case DrawCmdType::BorderRect:
            SetTexture(whiteTexture_, false);
            EmitBorder(cmd.x, cmd.y, cmd.w, cmd.h,
                       cmd.borderWidth, cmd.color);
            break;

        case DrawCmdType::TexturedRect: {
            rhi::RHITexture texToUse = whiteTexture_;
            bool isTextured = false;
            if (cmd.textureHandle != 0) {
                if (fontAtlasTexture_.IsValid() && cmd.textureHandle == fontAtlasTexture_.id) {
                    texToUse = fontAtlasTexture_;
                    isTextured = true;
                } else {
                    // Check if it's any other known texture (e.g. bold font atlas)
                    rhi::RHITexture otherTex{};
                    otherTex.id = cmd.textureHandle;
                    texToUse = otherTex;
                    isTextured = true;
                }
            }
            SetTexture(texToUse, isTextured);
            PushQuad(cmd.x, cmd.y, cmd.w, cmd.h,
                     cmd.u0, cmd.v0, cmd.u1, cmd.v1, cmd.color);
            break;
        }

        case DrawCmdType::RoundedRect:
            SetTexture(whiteTexture_, false);
            PushRoundedQuad(cmd.x, cmd.y, cmd.w, cmd.h,
                            cmd.cornerRadius, 0.0f, cmd.color);
            break;

        case DrawCmdType::RoundedBorderRect:
            SetTexture(whiteTexture_, false);
            PushRoundedQuad(cmd.x, cmd.y, cmd.w, cmd.h,
                            cmd.cornerRadius, cmd.borderWidth, cmd.color);
            break;

        case DrawCmdType::PushScissor: {
            Flush();
            int sx = cmd.scissorX;
            int sy = isVulkan_ ? cmd.scissorY
                               : (viewportH - cmd.scissorY - cmd.scissorH);
            int sw = cmd.scissorW;
            int sh = cmd.scissorH;
            if (!scissorStack_.empty()) {
                auto& cur = scissorStack_.back();
                int x1 = std::max(sx, cur.x);
                int y1 = std::max(sy, cur.y);
                int x2 = std::min(sx + sw, cur.x + cur.w);
                int y2 = std::min(sy + sh, cur.y + cur.h);
                sx = x1; sy = y1;
                sw = std::max(0, x2 - x1);
                sh = std::max(0, y2 - y1);
            }
            scissorStack_.push_back({sx, sy, sw, sh});
            rhi::RHIScissor sc{};
            sc.x = sx; sc.y = sy;
            sc.width  = static_cast<uint32_t>(sw);
            sc.height = static_cast<uint32_t>(sh);
            device_->SetScissor(sc);
            break;
        }

        case DrawCmdType::PopScissor:
            Flush();
            if (!scissorStack_.empty()) scissorStack_.pop_back();
            if (!scissorStack_.empty()) {
                auto& prev = scissorStack_.back();
                rhi::RHIScissor sc{};
                sc.x = prev.x; sc.y = prev.y;
                sc.width  = static_cast<uint32_t>(prev.w);
                sc.height = static_cast<uint32_t>(prev.h);
                device_->SetScissor(sc);
            } else {
                // Disable scissor by setting full-viewport rect.
                rhi::RHIScissor sc{};
                sc.x = 0; sc.y = 0;
                sc.width  = static_cast<uint32_t>(viewportW);
                sc.height = static_cast<uint32_t>(viewportH);
                device_->SetScissor(sc);
            }
            break;

        case DrawCmdType::Line: {
            SetTexture(whiteTexture_, false);
            float dx = cmd.w - cmd.x;
            float dy = cmd.h - cmd.y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len < 0.001f) break;
            float hw = cmd.borderWidth * 0.5f;
            float nx = -dy / len * hw;
            float ny =  dx / len * hw;
            PushTriangle(cmd.x + nx, cmd.y + ny,
                         cmd.x - nx, cmd.y - ny,
                         cmd.w + nx, cmd.h + ny, cmd.color);
            PushTriangle(cmd.x - nx, cmd.y - ny,
                         cmd.w - nx, cmd.h - ny,
                         cmd.w + nx, cmd.h + ny, cmd.color);
            break;
        }

        case DrawCmdType::FilledCircle: {
            SetTexture(whiteTexture_, false);
            float r = cmd.w;
            float side = r * 2.0f;
            float radii[4] = {r, r, r, r};
            PushRoundedQuad(cmd.x - r, cmd.y - r, side, side,
                            radii, 0.0f, cmd.color);
            break;
        }

        case DrawCmdType::CircleOutline: {
            SetTexture(whiteTexture_, false);
            float r = cmd.w;
            float side = r * 2.0f;
            float radii[4] = {r, r, r, r};
            PushRoundedQuad(cmd.x - r, cmd.y - r, side, side,
                            radii, cmd.borderWidth, cmd.color);
            break;
        }

        case DrawCmdType::Triangle:
            SetTexture(whiteTexture_, false);
            PushTriangle(cmd.x, cmd.y, cmd.w, cmd.h,
                         cmd.x2, cmd.y2, cmd.color);
            break;
        }
    }

    Flush();
}

// ====================================================================
// Private batching helpers
// ====================================================================

void UIRHIRenderer::SetTexture(rhi::RHITexture tex, bool isTextured) {
    if (tex.id != currentTexture_.id || isTextured != currentUseTexture_) {
        if (vertexCount_ > 0) Flush();
        currentTexture_    = tex;
        currentUseTexture_ = isTextured;
    }
}

void UIRHIRenderer::PushQuad(float x, float y, float w, float h,
                              float u0, float v0, float u1, float v1,
                              Color4 c) {
    if (vertexCount_ + 6 > MAX_VERTICES) Flush();

    UIVertex* v = &vertices_[vertexCount_];
    v[0] = {x,     y,     u0, v0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0}};
    v[1] = {x + w, y,     u1, v0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0}};
    v[2] = {x + w, y + h, u1, v1, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0}};
    v[3] = {x,     y,     u0, v0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0}};
    v[4] = {x + w, y + h, u1, v1, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0}};
    v[5] = {x,     y + h, u0, v1, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0}};
    vertexCount_ += 6;
}

void UIRHIRenderer::PushRoundedQuad(float x, float y, float w, float h,
                                     const float radii[4], float borderWidth,
                                     Color4 c) {
    if (vertexCount_ + 6 > MAX_VERTICES) Flush();

    UIVertex* v = &vertices_[vertexCount_];
    float halfW = w * 0.5f, halfH = h * 0.5f;
    float sdf[4] = {halfW, halfH, borderWidth, 0.0f};
    float rv[4]  = {radii[0], radii[1], radii[2], radii[3]};

    v[0] = {x,     y,     0.0f, 0.0f, c.r, c.g, c.b, c.a,
            {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]}};
    v[1] = {x + w, y,     1.0f, 0.0f, c.r, c.g, c.b, c.a,
            {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]}};
    v[2] = {x + w, y + h, 1.0f, 1.0f, c.r, c.g, c.b, c.a,
            {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]}};
    v[3] = {x,     y,     0.0f, 0.0f, c.r, c.g, c.b, c.a,
            {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]}};
    v[4] = {x + w, y + h, 1.0f, 1.0f, c.r, c.g, c.b, c.a,
            {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]}};
    v[5] = {x,     y + h, 0.0f, 1.0f, c.r, c.g, c.b, c.a,
            {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]}};
    vertexCount_ += 6;
}

void UIRHIRenderer::EmitBorder(float x, float y, float w, float h,
                                float bw, Color4 c) {
    PushQuad(x, y, w, bw, 0, 0, 1, 1, c);               // top
    PushQuad(x, y + h - bw, w, bw, 0, 0, 1, 1, c);      // bottom
    PushQuad(x, y + bw, bw, h - bw * 2.0f, 0, 0, 1, 1, c); // left
    PushQuad(x + w - bw, y + bw, bw, h - bw * 2.0f, 0, 0, 1, 1, c); // right
}

void UIRHIRenderer::PushTriangle(float x0, float y0, float x1, float y1,
                                  float ax2, float ay2, Color4 c) {
    if (vertexCount_ + 3 > MAX_VERTICES) Flush();
    UIVertex* v = &vertices_[vertexCount_];
    v[0] = {x0, y0, 0, 0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0}};
    v[1] = {x1, y1, 0, 0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0}};
    v[2] = {ax2, ay2, 0, 0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0}};
    vertexCount_ += 3;
}

void UIRHIRenderer::Flush() {
    if (vertexCount_ == 0) return;

    // On Vulkan, draw commands are deferred, so we need to write each
    // batch to a different VBO region to avoid overwriting data the GPU
    // hasn't consumed yet.  On OpenGL, commands execute immediately so
    // we can safely reuse offset 0 every time.
    uint32_t drawFirst = 0;
    size_t uploadOffset = 0;
    if (isVulkan_) {
        if (flushOffset_ + vertexCount_ > MAX_VERTICES) {
            vertexCount_ = 0;
            return;
        }
        uploadOffset = flushOffset_ * sizeof(UIVertex);
        drawFirst    = static_cast<uint32_t>(flushOffset_);
    }

    device_->UpdateBuffer(vbo_, vertices_,
                          vertexCount_ * sizeof(UIVertex), uploadOffset);
    device_->BindVertexBuffer(vbo_, 0, 0);

    // Push constants: viewport + useTexture
    UIPushConstants pc{};
    pc.viewportW  = static_cast<float>(viewportW_);
    pc.viewportH  = static_cast<float>(viewportH_);
    pc.useTexture = currentUseTexture_ ? 1 : 0;
    device_->PushConstants(rhi::RHIShaderStage::Vertex | rhi::RHIShaderStage::Fragment,
                           &pc, sizeof(pc), 0);

    // Bind texture
    device_->BindTexture(currentTexture_, 0, 0);

    // Draw
    device_->Draw(static_cast<uint32_t>(vertexCount_), 1, drawFirst);

    if (isVulkan_) flushOffset_ += vertexCount_;
    vertexCount_ = 0;
}

} // namespace ui
} // namespace koilo
