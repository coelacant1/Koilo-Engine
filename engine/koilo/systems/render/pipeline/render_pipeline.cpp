// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file render_pipeline.cpp
 * @brief Unified GPU render pipeline implementation.
 *
 * Single implementation of scene traversal, sky rendering, debug lines,
 * overlay compositing, and blit-to-screen using the RHI abstraction.
 * Replaces duplicated logic that previously lived in both the OpenGL
 * and Vulkan render backends).
 *
 * @date 03/19/2026
 * @author Coela Can't
 */

#include "render_pipeline.hpp"

#include <koilo/systems/render/rhi/rhi_device.hpp>
#include <koilo/systems/scene/scene.hpp>
#include <koilo/systems/scene/mesh.hpp>
#include <koilo/systems/scene/camera/camerabase.hpp>
#include <koilo/systems/scene/camera/cameralayout.hpp>
#include <koilo/systems/render/sky/sky.hpp>
#include <koilo/systems/render/material/imaterial.hpp>
#include <koilo/systems/render/material/implementations/kslmaterial.hpp>
#include <koilo/assets/image/texture.hpp>
#include <koilo/assets/model/itrianglegroup.hpp>
#include <koilo/core/math/matrix4x4.hpp>
#include <koilo/core/math/transform.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/vector2d.hpp>
#include <koilo/core/color/color888.hpp>
#include <koilo/debug/debugdraw.hpp>
#include <koilo/systems/render/canvas2d.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <koilo/ksl/ksl_registry.hpp>
#include <koilo/ksl/ksl_module.hpp>
#include <koilo/systems/profiling/performanceprofiler.hpp>

#include <cmath>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

namespace koilo::rhi {

// -- Constants ----------------------------------------------------------

static constexpr size_t kMaxDebugLineVerts  = 65536;
static constexpr float  kPi                 = 3.14159265358979323846f;

// -- Fullscreen quad geometry (NDC) -------------------------------------

static const FullscreenVertex kQuadVerts[6] = {
    {-1.0f, -1.0f, 0.0f, 0.0f},
    { 1.0f, -1.0f, 1.0f, 0.0f},
    { 1.0f,  1.0f, 1.0f, 1.0f},
    {-1.0f, -1.0f, 0.0f, 0.0f},
    { 1.0f,  1.0f, 1.0f, 1.0f},
    {-1.0f,  1.0f, 0.0f, 1.0f},
};

// -- Constructor / Destructor -------------------------------------------

RenderPipeline::RenderPipeline() = default;
RenderPipeline::~RenderPipeline() {
    if (initialized_) Shutdown();
}

// -- Configure ----------------------------------------------------------

void RenderPipeline::Configure(const RenderPipelineConfig& config) {
    config_ = config;
}

void RenderPipeline::OwnDevice(std::unique_ptr<IRHIDevice> device) {
    ownedDevice_ = std::move(device);
}

void RenderPipeline::OwnRegistry(std::unique_ptr<ksl::KSLRegistry> registry) {
    ownedRegistry_ = std::move(registry);
}

// -- IRenderBackend: Initialize -----------------------------------------

bool RenderPipeline::Initialize() {
    if (initialized_) return true;
    if (!config_.device) {
        KL_ERR("RenderPipeline", "No RHI device configured");
        return false;
    }

    auto* device = config_.device;

    // -- Shared caches -----------------------------------------------
    meshCache_     = std::make_unique<MeshCache>(device);
    textureCache_  = std::make_unique<TextureCache>(device);
    materialBinder_ = std::make_unique<MaterialBinder>(device);
    materialBinder_->SetPipelineResolver(
        [this](const std::string& name, const RenderFlags* flags) {
            return GetOrCreateScenePipeline(name, flags);
        }
    );

    // -- Fullscreen quad VBO -----------------------------------------
    {
        RHIBufferDesc desc{};
        desc.size        = sizeof(kQuadVerts);
        desc.usage       = RHIBufferUsage::Vertex;
        desc.hostVisible = true;
        desc.debugName   = "Pipeline.QuadVBO";
        fullscreenQuadVBO_ = device->CreateBuffer(desc);
        if (!fullscreenQuadVBO_.IsValid()) {
            KL_ERR("RenderPipeline", "Failed to create fullscreen quad VBO");
            return false;
        }
        device->UpdateBuffer(fullscreenQuadVBO_, kQuadVerts, sizeof(kQuadVerts));
    }

    // -- Scene-format quad VBO (RHIVertex for sky/effects through scene pipeline)
    {
        static const RHIVertex kSceneQuad[6] = {
            {-1, -1, 0.5f,  0, 0, 1,  0, 0},
            { 1, -1, 0.5f,  0, 0, 1,  1, 0},
            { 1,  1, 0.5f,  0, 0, 1,  1, 1},
            {-1, -1, 0.5f,  0, 0, 1,  0, 0},
            { 1,  1, 0.5f,  0, 0, 1,  1, 1},
            {-1,  1, 0.5f,  0, 0, 1,  0, 1},
        };
        RHIBufferDesc desc{};
        desc.size        = sizeof(kSceneQuad);
        desc.usage       = RHIBufferUsage::Vertex;
        desc.hostVisible = true;
        desc.debugName   = "Pipeline.SceneQuadVBO";
        sceneQuadVBO_ = device->CreateBuffer(desc);
        if (sceneQuadVBO_.IsValid())
            device->UpdateBuffer(sceneQuadVBO_, kSceneQuad, sizeof(kSceneQuad));
    }

    // -- Debug line VBO (dynamic, allocated on first use) ------------
    {
        RHIBufferDesc desc{};
        desc.size        = kMaxDebugLineVerts * sizeof(DebugLineVertex);
        desc.usage       = RHIBufferUsage::Vertex;
        desc.hostVisible = true;
        desc.debugName   = "Pipeline.DebugLineVBO";
        debugLineVBO_ = device->CreateBuffer(desc);
    }

    // -- Uniform buffers ---------------------------------------------
    {
        RHIBufferDesc desc{};
        desc.usage       = RHIBufferUsage::Uniform;
        desc.hostVisible = true;

        desc.size      = sizeof(TransformUBO);
        desc.debugName = "Pipeline.TransformUBO";
        transformUBO_  = device->CreateBuffer(desc);

        // Dedicated UBO for the sky pass (B1) - populated at most once
        // per frame with identity model/view/proj + scene's
        // inverseViewProj.  Avoids overwriting transformUBO_ and the
        // matching restore upload that the original code performed.
        desc.size        = sizeof(TransformUBO);
        desc.debugName   = "Pipeline.SkyTransformUBO";
        skyTransformUBO_ = device->CreateBuffer(desc);

        desc.size      = sizeof(SceneUBO);
        desc.debugName = "Pipeline.SceneUBO";
        sceneUBO_      = device->CreateBuffer(desc);

        desc.size      = sizeof(GPULight) * kMaxGPULights;
        desc.debugName = "Pipeline.LightSSBO";
        desc.usage     = RHIBufferUsage::Storage;
        lightSSBO_     = device->CreateBuffer(desc);

        // Dummy audio SSBO (binding 3) -- required by scene set layout
        desc.size      = 16; // minimum
        desc.debugName = "Pipeline.AudioSSBO";
        desc.usage     = RHIBufferUsage::Storage;
        audioSSBO_     = device->CreateBuffer(desc);

        // Overlay UBO: single float (alpha)
        desc.size      = 16; // std140 minimum
        desc.usage     = RHIBufferUsage::Uniform;
        desc.debugName = "Pipeline.OverlayUBO";
        overlayUBO_    = device->CreateBuffer(desc);
    }

    // -- Off-screen render target ------------------------------------
    EnsureOffscreenTarget(config_.initialWidth, config_.initialHeight);

    // -- Built-in pipelines ------------------------------------------
    CreateBuiltinPipelines();

    initialized_ = true;
    KL_LOG("RenderPipeline", "Initialized successfully (%s)",
           config_.device->GetName());
    return true;
}

// -- IRenderBackend: Shutdown -------------------------------------------

void RenderPipeline::Shutdown() {
    if (!initialized_) return;
    auto* device = config_.device;
    if (!device) return;

    KL_LOG("RenderPipeline", "Shutdown: clearing caches...");
    // Destroy shared caches (they destroy their own RHI resources)
    materialBinder_.reset();
    textureCache_.reset();
    meshCache_.reset();

    KL_LOG("RenderPipeline", "Shutdown: destroying pipelines...");
    // Destroy all pipelines (scene + built-in)
    DestroyAllPipelines();

    KL_LOG("RenderPipeline", "Shutdown: destroying shaders...");
    // Destroy built-in shaders
    auto destroyShader = [&](RHIShader& s) {
        if (s.IsValid()) { device->DestroyShader(s); s = {}; }
    };
    destroyShader(blitVertShader_);
    destroyShader(blitFragShader_);
    destroyShader(overlayFragShader_);
    destroyShader(debugLineVertShader_);
    destroyShader(debugLineFragShader_);
    destroyShader(pinkVertShader_);
    destroyShader(pinkFragShader_);

    KL_LOG("RenderPipeline", "Shutdown: destroying buffers...");
    // Destroy buffers
    auto destroyBuffer = [&](RHIBuffer& b) {
        if (b.IsValid()) { device->DestroyBuffer(b); b = {}; }
    };
    destroyBuffer(fullscreenQuadVBO_);
    destroyBuffer(sceneQuadVBO_);
    destroyBuffer(debugLineVBO_);
    destroyBuffer(transformUBO_);
    destroyBuffer(skyTransformUBO_);
    destroyBuffer(sceneUBO_);
    destroyBuffer(lightSSBO_);
    destroyBuffer(audioSSBO_);
    destroyBuffer(overlayUBO_);

    KL_LOG("RenderPipeline", "Shutdown: destroying offscreen target...");
    // Destroy off-screen target
    DestroyOffscreenTarget();

    // Release per-canvas overlay textures.
    for (auto& kv : overlayStates_) {
        if (kv.second.texture.IsValid())
            config_.device->DestroyTexture(kv.second.texture);
    }
    overlayStates_.clear();
    overlayStagingRGBA_.clear();
    overlayStagingRGBA_.shrink_to_fit();

    // Reset light upload cache (B2) so a fresh Initialize() re-uploads.
    lastUploadedLightCount_ = -1;

    KL_LOG("RenderPipeline", "Shutdown: shutting down device...");
    // Shutdown owned device (after all GPU resources are released)
    if (ownedDevice_) {
        ownedDevice_->Shutdown();
        KL_LOG("RenderPipeline", "Shutdown: releasing device...");
        ownedDevice_.reset();
    }

    KL_LOG("RenderPipeline", "Shutdown: releasing registry...");
    // Clear static registry pointer before destroying it so late-running
    // KSLMaterial destructors (from script engine cleanup) won't call into
    // freed KSLModule objects.
    KSLMaterial::SetRegistry(nullptr);
    ownedRegistry_.reset();

    initialized_ = false;
    KL_LOG("RenderPipeline", "Shutdown complete");
}

bool RenderPipeline::IsInitialized() const { return initialized_; }
const char* RenderPipeline::GetName() const { return "RHI RenderPipeline"; }

bool RenderPipeline::UsesTopLeftOrigin() const {
    if (!config_.device) return false;
    const char* name = config_.device->GetName();
    if (!name) return false;
    std::string n(name);
    // Vulkan and Software RHI both use top-left origin; OpenGL uses bottom-left.
    return n.find("Vulkan") != std::string::npos ||
           n.find("Software") != std::string::npos;
}

// -- IGPURenderBackend: PrepareFrame / FinishFrame ----------------------

void RenderPipeline::PrepareFrame() {
    if (!initialized_) return;
    overlayStagedThisFrame_ = false;
    config_.device->BeginFrame();
}

void RenderPipeline::FinishFrame() {
    if (!initialized_) return;
    config_.device->EndFrame();
    config_.device->Present();
}

// -- IRenderBackend: Render (single-call convenience) -------------------

void RenderPipeline::Render(Scene* scene, CameraBase* camera) {
    if (!BeginScenePass(scene, camera))
        return;
    RenderSky();
    RenderSceneMeshes();
    RenderSceneDebugLines();
    EndScenePass();
}

void RenderPipeline::ReadPixels(Color888* /*buffer*/, int /*width*/, int /*height*/) {
    // Readback not yet implemented via RHI -- requires staging buffer support.
}

// -- Granular render methods (for render graph) -------------------------

bool RenderPipeline::BeginScenePass(Scene* scene, CameraBase* camera) {
    if (!initialized_ || !scene || !camera || camera->Is2D())
        return false;

    KL_PERF_SCOPE("RenderPipeline.BeginScenePass");

    auto* device = config_.device;

    // -- Viewport from camera ----------------------------------------
    Vector2D minC = camera->GetCameraMinCoordinate();
    Vector2D maxC = camera->GetCameraMaxCoordinate();
    uint32_t vpW  = static_cast<uint32_t>(maxC.X - minC.X + 1);
    uint32_t vpH  = static_cast<uint32_t>(maxC.Y - minC.Y + 1);

    if (vpW == 0 || vpH == 0) return false;

    // -- Ensure off-screen render target matches ---------------------
    EnsureOffscreenTarget(vpW, vpH);

    // -- Begin off-screen render pass --------------------------------
    RHIClearValue clear{};
    clear.color[0] = 0.0f; clear.color[1] = 0.0f;
    clear.color[2] = 0.0f; clear.color[3] = 1.0f;
    clear.depth    = 1.0f;
    clear.stencil  = 0;

    device->BeginRenderPass(offscreenPass_, offscreenFB_, clear);

    // -- Viewport & scissor ------------------------------------------
    RHIViewport vp{};
    if (config_.vulkanDepthRemap) {
        vp.x      = 0.0f;
        vp.y      = static_cast<float>(vpH);
        vp.width  = static_cast<float>(vpW);
        vp.height = -static_cast<float>(vpH);
    } else {
        vp.x      = 0.0f;
        vp.y      = 0.0f;
        vp.width  = static_cast<float>(vpW);
        vp.height = static_cast<float>(vpH);
    }
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    device->SetViewport(vp);

    RHIScissor sc{};
    sc.x      = 0;
    sc.y      = 0;
    sc.width  = vpW;
    sc.height = vpH;
    device->SetScissor(sc);

    // -- Build view/projection matrices ------------------------------
    BuildViewProjection(camera, sceneView_, sceneProj_, sceneCameraPos_);

    // Compute inverseViewProj for shaders that reconstruct view direction
    Matrix4x4 viewMat, projMat;
    std::memcpy(&viewMat.M[0][0], sceneView_, 64);
    std::memcpy(&projMat.M[0][0], sceneProj_, 64);
    Matrix4x4 vpMat = projMat * viewMat;
    Matrix4x4 invVP = vpMat.Inverse();
    {
        Matrix4x4 invVPT = invVP.Transpose();
        std::memcpy(sceneInvVP_, &invVPT.M[0][0], 64);
    }

    // -- Upload transform UBO ----------------------------------------
    {
        TransformUBO ubo{};
        for (int i = 0; i < 4; ++i) ubo.model[i * 4 + i] = 1.0f;
        std::memcpy(ubo.view, sceneView_, sizeof(ubo.view));
        std::memcpy(ubo.projection, sceneProj_, sizeof(ubo.projection));
        std::memcpy(ubo.cameraPos, sceneCameraPos_, sizeof(ubo.cameraPos));
        std::memcpy(ubo.inverseViewProj, sceneInvVP_, sizeof(ubo.inverseViewProj));
        device->UpdateBuffer(transformUBO_, &ubo, sizeof(ubo));
    }

    // Bind transform UBO (set 0, binding 0)
    device->BindUniformBuffer(transformUBO_, 0, 0);

    // -- Upload lights (must precede any Draw so all set 0 bindings
    //    are populated before the first descriptor set flush) --------
    UploadLights(scene);

    currentScene_    = scene;
    scenePassActive_ = true;
    return true;
}

void RenderPipeline::RenderSky() {
    if (!scenePassActive_) return;

    auto* device = config_.device;
    Sky& sky = Sky::GetInstance();
    if (!sky.IsEnabled()) return;

    KSLMaterial* kmat = sky.GetMaterial();
    if (!kmat || !kmat->IsBound() || !kmat->GetModule()) return;

    const MaterialBinding* binding = materialBinder_->Bind(kmat);
    if (!binding || !binding->valid) return;

    materialBinder_->UpdateUniforms(kmat);

    // Identity transform for sky (screen-space quad).
    // Uses a dedicated UBO so the scene's transformUBO_ is left intact
    // - no per-frame "restore" upload needed (B1).
    TransformUBO skyUbo{};
    for (int i = 0; i < 4; ++i) {
        skyUbo.model[i * 4 + i]      = 1.0f;
        skyUbo.view[i * 4 + i]       = 1.0f;
        skyUbo.projection[i * 4 + i] = 1.0f;
    }
    std::memcpy(skyUbo.inverseViewProj, sceneInvVP_,
                sizeof(skyUbo.inverseViewProj));
    device->UpdateBuffer(skyTransformUBO_, &skyUbo, sizeof(skyUbo));
    device->BindUniformBuffer(skyTransformUBO_, 0, 0);

    device->BindPipeline(binding->pipeline);
    device->BindUniformBuffer(binding->uniformBuffer, 1, 0,
                              0, binding->uniformSize);

    if (kmat->TextureCount() > 0) {
        Texture* tex = kmat->GetTexture(0);
        if (tex) {
            RHITexture gpuTex = textureCache_->GetOrUpload(tex);
            if (gpuTex.IsValid())
                device->BindTexture(gpuTex, 2, 0);
        }
    }

    device->BindVertexBuffer(sceneQuadVBO_);
    device->Draw(6);

    // Restore camera transform binding for subsequent draws.  The
    // contents of transformUBO_ were not modified by the sky pass, so a
    // simple rebind is sufficient - no UpdateBuffer required (B1).
    device->BindUniformBuffer(transformUBO_, 0, 0);
}

void RenderPipeline::RenderSceneMeshes() {
    if (!scenePassActive_) return;
    RenderMeshes(currentScene_, sceneView_, sceneProj_, sceneCameraPos_);
}

void RenderPipeline::RenderSceneDebugLines() {
    if (!scenePassActive_) return;
    RenderDebugLines(sceneView_, sceneProj_);
}

void RenderPipeline::EndScenePass() {
    if (!scenePassActive_) return;
    config_.device->EndRenderPass();
    scenePassActive_ = false;
    currentScene_    = nullptr;
}

// -- IGPURenderBackend: BlitToScreen ------------------------------------

void RenderPipeline::BlitToScreen(int screenW, int screenH) {
    if (!initialized_ || !offscreenColor_.IsValid()) return;
    if (!blitPipeline_.IsValid() || !fullscreenQuadVBO_.IsValid()) return;

    auto* device = config_.device;

    // Begin the swapchain render pass (device manages FB internally)
    RHIClearValue clear{};
    clear.color[0] = 0.0f; clear.color[1] = 0.0f;
    clear.color[2] = 0.0f; clear.color[3] = 1.0f;
    device->BeginSwapchainRenderPass(clear);

    // Set swapchain viewport/scissor
    RHIViewport vp{};
    vp.width    = static_cast<float>(screenW);
    vp.height   = static_cast<float>(screenH);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    device->SetViewport(vp);

    RHIScissor sc{};
    sc.width  = static_cast<uint32_t>(screenW);
    sc.height = static_cast<uint32_t>(screenH);
    device->SetScissor(sc);

    DrawFullscreenQuad(blitPipeline_, offscreenColor_);

    // NOTE: swapchain render pass is left open for overlay/UI rendering.
    // It is ended by VulkanBackend::SwapOnly() (or the OpenGL equivalent).
}

// -- IGPURenderBackend: CompositeCanvasOverlays -------------------------

void RenderPipeline::StageCanvasOverlays(int /*screenW*/, int /*screenH*/) {
    auto& canvases = Canvas2D::ActiveList();
    if (!initialized_) return;
    if (!overlayPipeline_.IsValid()) return;

    auto* device = config_.device;
    const bool subUpdateOK = device->SupportsTextureSubUpdate();

    // Drop GPU resources for canvases no longer attached.
    if (!overlayStates_.empty()) {
        for (auto it = overlayStates_.begin(); it != overlayStates_.end();) {
            bool stillActive = false;
            for (auto* c : canvases) { if (c == it->first) { stillActive = true; break; } }
            if (!stillActive) {
                if (it->second.texture.IsValid()) device->DestroyTexture(it->second.texture);
                it = overlayStates_.erase(it);
            } else {
                ++it;
            }
        }
    }

    overlayStagedThisFrame_ = true;
    if (canvases.empty()) return;

    for (auto* canvas : canvases) {
        if (!canvas) continue;

        const int cw = canvas->GetWidth();
        const int ch = canvas->GetHeight();
        if (cw <= 0 || ch <= 0) continue;

        const uint8_t* rgba = canvas->GetRGBA8();
        if (!rgba) continue;

        auto& st = overlayStates_[canvas];
        const uint32_t ucw = static_cast<uint32_t>(cw);
        const uint32_t uch = static_cast<uint32_t>(ch);

        // (Re)create texture on dim change. Resets upload state - first
        // upload after this must be a full upload (Vulkan needs valid
        // initial layout, others just need full coverage).
        if (!st.texture.IsValid() || st.texW != ucw || st.texH != uch) {
            if (st.texture.IsValid()) {
                device->DestroyTexture(st.texture);
                st.texture = {};
            }
            RHITextureDesc texDesc{};
            texDesc.width     = ucw;
            texDesc.height    = uch;
            texDesc.format    = RHIFormat::RGBA8_Unorm;
            texDesc.usage     = RHITextureUsage::Sampled | RHITextureUsage::TransferDst;
            texDesc.filter    = RHISamplerFilter::Nearest;
            texDesc.debugName = "Pipeline.CanvasOverlay";

            st.texture = device->CreateTexture(texDesc);
            if (!st.texture.IsValid()) continue;

            st.texW        = ucw;
            st.texH        = uch;
            st.hasUploaded = false;
            st.lastModVer  = ~uint64_t{0};
            st.lastMinX = 0; st.lastMinY = 0; st.lastMaxX = -1; st.lastMaxY = -1;
        }

        const uint64_t modVer = canvas->GetModVersion();
        const bool dirty      = canvas->IsDirty();
        const bool prevDirty  = (st.lastMaxX >= 0);

        // Skip upload entirely when the canvas hasn't been touched since
        // the last upload AND we have valid GPU contents.
        if (st.hasUploaded && modVer == st.lastModVer) {
            continue;
        }

        bool uploadOK = true;
        if (!st.hasUploaded || !subUpdateOK) {
            // Full upload - Canvas2D already stores packed RGBA8 in the
            // exact GPU layout, so no per-pixel conversion is needed.
            const size_t bytes = static_cast<size_t>(cw) * ch * 4;
            uploadOK = device->StageTextureFull(st.texture, rgba, bytes, st.texW, st.texH);
            if (uploadOK) st.hasUploaded = true;
        } else if (dirty || prevDirty) {
            // Sub-rect upload: union(prev rect, current rect) so that
            // pixels cleared since the last upload (alpha already set
            // to 0 by Canvas2D::Clear over its prior dirty rect) are
            // also re-uploaded. Both rects intersected with [0,w)x[0,h).
            int rMinX = cw, rMinY = ch, rMaxX = -1, rMaxY = -1;
            if (dirty) {
                rMinX = std::min(rMinX, canvas->GetDirtyMinX());
                rMinY = std::min(rMinY, canvas->GetDirtyMinY());
                rMaxX = std::max(rMaxX, canvas->GetDirtyMaxX());
                rMaxY = std::max(rMaxY, canvas->GetDirtyMaxY());
            }
            if (prevDirty) {
                rMinX = std::min(rMinX, st.lastMinX);
                rMinY = std::min(rMinY, st.lastMinY);
                rMaxX = std::max(rMaxX, st.lastMaxX);
                rMaxY = std::max(rMaxY, st.lastMaxY);
            }
            // Clamp to texture bounds.
            if (rMinX < 0) rMinX = 0;
            if (rMinY < 0) rMinY = 0;
            if (rMaxX >= cw) rMaxX = cw - 1;
            if (rMaxY >= ch) rMaxY = ch - 1;

            if (rMaxX >= rMinX && rMaxY >= rMinY) {
                const uint32_t rx = static_cast<uint32_t>(rMinX);
                const uint32_t ry = static_cast<uint32_t>(rMinY);
                const uint32_t rw = static_cast<uint32_t>(rMaxX - rMinX + 1);
                const uint32_t rh = static_cast<uint32_t>(rMaxY - rMinY + 1);
                const size_t   bytes = static_cast<size_t>(rw) * rh * 4;
                // Repack the dirty sub-rect into a tightly-packed scratch
                // buffer (rows of width=rw rather than the canvas's stride).
                if (overlayStagingRGBA_.size() < bytes) overlayStagingRGBA_.resize(bytes);
                uint8_t* dst = overlayStagingRGBA_.data();
                const size_t srcStride = static_cast<size_t>(cw) * 4;
                const size_t dstStride = static_cast<size_t>(rw) * 4;
                const uint8_t* srcBase  = rgba + ry * srcStride + rx * 4;
                for (uint32_t row = 0; row < rh; ++row) {
                    std::memcpy(dst + row * dstStride,
                                srcBase + row * srcStride,
                                dstStride);
                }
                uploadOK = device->StageTextureRegion(st.texture, dst, bytes, rx, ry, rw, rh);
            }
        }

        // Only advance dirty bookkeeping when the upload succeeded - otherwise
        // GPU contents may be stale and we want a re-upload next frame.
        if (!uploadOK) continue;

        // Snapshot the *current* dirty rect (not the union) so that next
        // frame we can re-upload it if the user clears it.
        if (dirty) {
            st.lastMinX = canvas->GetDirtyMinX();
            st.lastMinY = canvas->GetDirtyMinY();
            st.lastMaxX = canvas->GetDirtyMaxX();
            st.lastMaxY = canvas->GetDirtyMaxY();
        } else {
            st.lastMinX = 0; st.lastMinY = 0; st.lastMaxX = -1; st.lastMaxY = -1;
        }
        st.lastModVer = modVer;
    }
}

void RenderPipeline::CompositeCanvasOverlays(int screenW, int screenH) {
    auto& canvases = Canvas2D::ActiveList();
    if (!initialized_) return;
    if (!overlayPipeline_.IsValid() || !fullscreenQuadVBO_.IsValid()) return;

    // For backward compat with direct callers (non-render-graph paths) and
    // backends that don't use the canvas_stage graph pass: stage uploads
    // here on first call per frame.  When the frame composer's canvas_stage
    // pass already ran (Vulkan path), this is a no-op.
    if (!overlayStagedThisFrame_) {
        StageCanvasOverlays(screenW, screenH);
    }

    if (canvases.empty()) return;

    for (auto* canvas : canvases) {
        if (!canvas) continue;
        auto it = overlayStates_.find(canvas);
        if (it == overlayStates_.end()) continue;
        const auto& st = it->second;
        if (!st.texture.IsValid() || !st.hasUploaded) continue;
        DrawFullscreenQuad(overlayPipeline_, st.texture, 1.0f);
    }
}

// -- Off-screen render target management --------------------------------

void RenderPipeline::EnsureOffscreenTarget(uint32_t width, uint32_t height) {
    if (width == offscreenWidth_ && height == offscreenHeight_
        && offscreenFB_.IsValid()) {
        return;
    }

    auto* device = config_.device;
    DestroyOffscreenTarget();

    // Color texture
    {
        RHITextureDesc desc{};
        desc.width   = width;
        desc.height  = height;
        desc.format  = RHIFormat::RGBA8_Unorm;
        desc.usage   = RHITextureUsage::RenderTarget | RHITextureUsage::Sampled;
        desc.debugName = "Pipeline.OffscreenColor";
        offscreenColor_ = device->CreateTexture(desc);
    }

    // Depth texture
    {
        RHITextureDesc desc{};
        desc.width   = width;
        desc.height  = height;
        desc.format  = RHIFormat::D24_Unorm_S8_Uint;
        desc.usage   = RHITextureUsage::DepthStencil;
        desc.debugName = "Pipeline.OffscreenDepth";
        offscreenDepth_ = device->CreateTexture(desc);
    }

    // Render pass
    {
        RHIRenderPassDesc desc{};
        desc.colorAttachmentCount = 1;
        desc.colorAttachments[0].format  = RHIFormat::RGBA8_Unorm;
        desc.colorAttachments[0].loadOp  = RHILoadOp::Clear;
        desc.colorAttachments[0].storeOp = RHIStoreOp::Store;
        desc.colorAttachments[0].sampleAfterPass = true; // blit shader samples this
        desc.hasDepth      = true;
        desc.depthFormat   = RHIFormat::D24_Unorm_S8_Uint;
        desc.depthLoadOp   = RHILoadOp::Clear;
        desc.depthStoreOp  = RHIStoreOp::DontCare;
        offscreenPass_ = device->CreateRenderPass(desc);
    }

    // Framebuffer
    offscreenFB_ = device->CreateFramebuffer(
        offscreenPass_, &offscreenColor_, 1, offscreenDepth_, width, height);

    offscreenWidth_  = width;
    offscreenHeight_ = height;
}

void RenderPipeline::DestroyOffscreenTarget() {
    auto* device = config_.device;
    if (!device) return;

    if (offscreenFB_.IsValid())    { device->DestroyFramebuffer(offscreenFB_);  offscreenFB_ = {}; }
    if (offscreenPass_.IsValid())  { device->DestroyRenderPass(offscreenPass_); offscreenPass_ = {}; }
    if (offscreenDepth_.IsValid()) { device->DestroyTexture(offscreenDepth_);   offscreenDepth_ = {}; }
    if (offscreenColor_.IsValid()) { device->DestroyTexture(offscreenColor_);   offscreenColor_ = {}; }

    offscreenWidth_  = 0;
    offscreenHeight_ = 0;
}

// -- Pipeline creation / caching ----------------------------------------

RHIPipeline RenderPipeline::GetOrCreateScenePipeline(const std::string& shaderName,
                                                      const RenderFlags* flags) {
    // B5: numeric POD key avoids string concat / heap alloc per lookup.
    uint16_t flagBits = 0;
    if (flags) {
        if (flags->depthStencil.depthTest)  flagBits |= 0x1;
        if (flags->depthStencil.depthWrite) flagBits |= 0x2;
        flagBits |= (static_cast<uint16_t>(flags->cullMode) & 0xF) << 2;
    }

    // Intern shader name -> stable id (one-time per unique shader)
    uint32_t shaderId;
    {
        auto sit = shaderIdMap_.find(shaderName);
        if (sit != shaderIdMap_.end()) {
            shaderId = sit->second;
        } else {
            shaderId = nextShaderId_++;
            shaderIdMap_.emplace(shaderName, shaderId);
        }
    }
    PipelineKey cacheKey{shaderId, flagBits};

    // Check cache
    auto it = scenePipelineCache_.find(cacheKey);
    if (it != scenePipelineCache_.end()) return it->second;

    // Resolve shader bytecode via factory-provided callback
    if (!config_.shaderResolver) {
        KL_WARN("RenderPipeline", "No shader resolver configured");
        return pinkErrorPipeline_;
    }

    ShaderData sd = config_.shaderResolver(shaderName);
    if (!sd.vertexCode || !sd.fragCode || sd.vertexCodeSize == 0 || sd.fragCodeSize == 0) {
        KL_WARN("RenderPipeline", "Shader '%s' not resolved -- using pink fallback",
                shaderName.c_str());
        return pinkErrorPipeline_;
    }

    auto* device = config_.device;

    RHIShader vertShader = device->CreateShader(
        RHIShaderStage::Vertex, sd.vertexCode, sd.vertexCodeSize);
    RHIShader fragShader = device->CreateShader(
        RHIShaderStage::Fragment, sd.fragCode, sd.fragCodeSize);

    if (!vertShader.IsValid() || !fragShader.IsValid()) {
        KL_WARN("RenderPipeline", "Failed to compile shader '%s'", shaderName.c_str());
        if (vertShader.IsValid()) device->DestroyShader(vertShader);
        if (fragShader.IsValid()) device->DestroyShader(fragShader);
        return pinkErrorPipeline_;
    }

    // Build pipeline descriptor for scene rendering
    RHIPipelineDesc desc{};
    desc.vertexShader   = vertShader;
    desc.fragmentShader = fragShader;
    desc.renderPass     = offscreenPass_;

    // RHIVertex layout: pos3 + normal3 + uv2 (32 bytes)
    desc.vertexAttrCount = 3;
    desc.vertexStride    = sizeof(RHIVertex);
    desc.vertexAttrs[0]  = {0, RHIFormat::RGB32F, 0};                            // position: 3 floats (12 bytes)
    desc.vertexAttrs[1]  = {1, RHIFormat::RGB32F, 12};                           // normal: 3 floats (12 bytes)
    desc.vertexAttrs[2]  = {2, RHIFormat::RG32F,  24};                           // uv: 2 floats (8 bytes)

    desc.topology    = RHITopology::TriangleList;
    desc.rasterizer  = {RHICullMode::Back, RHIFrontFace::CounterClockwise, RHIPolygonMode::Fill, false};
    desc.depthStencil = {true, true, RHICompareOp::Less, false};

    // Apply metadata-driven render flags if provided
    if (flags) {
        desc.depthStencil.depthTest  = flags->depthStencil.depthTest;
        desc.depthStencil.depthWrite = flags->depthStencil.depthWrite;
        desc.rasterizer.cullMode     = flags->cullMode;
    }

    desc.blend       = {};  // No blending for opaque scene geometry
    desc.debugName   = shaderName.c_str();

    // Populate material parameter metadata for OpenGL name-based bridging.
    if (config_.shaderRegistry) {
        ksl::KSLModule* mod = config_.shaderRegistry->GetModule(shaderName);
        if (mod) {
            ksl::ParamList params = mod->GetParams();
            uint32_t count = static_cast<uint32_t>(
                std::min(params.count, static_cast<int>(RHIPipelineDesc::kMaxMaterialParams)));
            for (uint32_t i = 0; i < count; ++i) {
                desc.materialParams[i].name = params.decls[i].name;
                desc.materialParams[i].type = static_cast<uint8_t>(params.decls[i].type);
            }
            desc.materialParamCount = count;
        }
    }

    RHIPipeline pipeline = device->CreatePipeline(desc);

    // Cache even if invalid (avoid repeated creation attempts)
    scenePipelineCache_[cacheKey] = pipeline;

    // Note: shader modules can be destroyed after pipeline creation
    // if the backend copies them. Keep them alive for safety.
    // They will be destroyed in DestroyAllPipelines.

    if (!pipeline.IsValid()) {
        KL_WARN("RenderPipeline", "Pipeline creation failed for '%s'",
                shaderName.c_str());
        return pinkErrorPipeline_;
    }

    return pipeline;
}

void RenderPipeline::CreateBuiltinPipelines() {
    // Built-in pipelines require shader resolver or embedded shaders.
    // During Phase 17g (migration), the factory will provide the resolver
    // with built-in SPIR-V/GLSL. For now, we mark pipelines as invalid
    // and use graceful fallbacks (no crash, just no rendering).
    //
    // The pipeline still functions correctly for scene shaders resolved
    // via the ShaderResolver callback, which is the primary rendering path.

    if (!config_.shaderResolver) return;

    auto* device = config_.device;

    // Get the swapchain render pass for blit/overlay pipelines.
    // These pipelines render to the screen, not the offscreen target.
    RHIRenderPass swapPass = device->GetSwapchainRenderPass();
    if (!swapPass.IsValid()) {
        // Fall back to offscreen pass (won't be compatible at runtime
        // but avoids a crash during pipeline creation)
        swapPass = offscreenPass_;
    }
    swapchainPass_ = swapPass;

    // Try to resolve built-in shaders by convention name
    auto tryCreateBuiltin = [&](const std::string& name,
                                RHIShader& outVert, RHIShader& outFrag,
                                RHIPipeline& outPipeline,
                                const RHIPipelineDesc& baseDesc) {
        ShaderData sd = config_.shaderResolver(name);
        if (!sd.vertexCode || !sd.fragCode) return;

        outVert = device->CreateShader(RHIShaderStage::Vertex,
                                       sd.vertexCode, sd.vertexCodeSize);
        outFrag = device->CreateShader(RHIShaderStage::Fragment,
                                       sd.fragCode, sd.fragCodeSize);
        if (!outVert.IsValid() || !outFrag.IsValid()) return;

        RHIPipelineDesc desc = baseDesc;
        desc.vertexShader   = outVert;
        desc.fragmentShader = outFrag;
        outPipeline = device->CreatePipeline(desc);
    };

    // -- Blit pipeline (fullscreen quad, no depth) -------------------
    {
        RHIPipelineDesc desc{};
        desc.renderPass     = swapchainPass_;
        desc.vertexAttrCount = 2;
        desc.vertexStride    = sizeof(FullscreenVertex);
        desc.vertexAttrs[0]  = {0, RHIFormat::RG32F, 0};  // position (vec2)
        desc.vertexAttrs[1]  = {1, RHIFormat::RG32F, 8};  // uv (vec2)
        desc.topology        = RHITopology::TriangleList;
        desc.rasterizer      = {RHICullMode::None, RHIFrontFace::CounterClockwise,
                                RHIPolygonMode::Fill, false};
        desc.depthStencil    = {false, false, RHICompareOp::Always, false};
        desc.layoutHint      = RHIPipelineDesc::LayoutHint::Blit;
        desc.debugName       = "__blit__";

        tryCreateBuiltin("__blit__", blitVertShader_, blitFragShader_,
                         blitPipeline_, desc);
    }

    // -- Overlay pipeline (same as blit + alpha blending) ------------
    {
        RHIPipelineDesc desc{};
        desc.renderPass      = swapchainPass_;
        desc.vertexAttrCount = 2;
        desc.vertexStride    = sizeof(FullscreenVertex);
        desc.vertexAttrs[0]  = {0, RHIFormat::RG32F, 0};
        desc.vertexAttrs[1]  = {1, RHIFormat::RG32F, 8};
        desc.topology        = RHITopology::TriangleList;
        desc.rasterizer      = {RHICullMode::None, RHIFrontFace::CounterClockwise,
                                RHIPolygonMode::Fill, false};
        desc.depthStencil    = {false, false, RHICompareOp::Always, false};
        desc.blend.enabled   = true;
        desc.blend.srcColor  = RHIBlendFactor::SrcAlpha;
        desc.blend.dstColor  = RHIBlendFactor::OneMinusSrcAlpha;
        desc.blend.colorOp   = RHIBlendOp::Add;
        desc.blend.srcAlpha  = RHIBlendFactor::One;
        desc.blend.dstAlpha  = RHIBlendFactor::OneMinusSrcAlpha;
        desc.blend.alphaOp   = RHIBlendOp::Add;
        desc.layoutHint      = RHIPipelineDesc::LayoutHint::Blit;
        desc.debugName       = "__overlay__";

        tryCreateBuiltin("__overlay__", blitVertShader_, overlayFragShader_,
                         overlayPipeline_, desc);
    }

    // -- Debug line pipeline -----------------------------------------
    {
        RHIPipelineDesc desc{};
        desc.renderPass      = offscreenPass_;
        desc.vertexAttrCount = 2;
        desc.vertexStride    = sizeof(DebugLineVertex);
        desc.vertexAttrs[0]  = {0, RHIFormat::RGB32F,  0};      // position (vec3, 12 bytes)
        desc.vertexAttrs[1]  = {1, RHIFormat::RGBA32F, 12};     // color (vec4, 16 bytes)
        desc.topology        = RHITopology::LineList;
        desc.rasterizer      = {RHICullMode::None, RHIFrontFace::CounterClockwise,
                                RHIPolygonMode::Fill, false};
        desc.depthStencil    = {true, false, RHICompareOp::Less, false};
        desc.debugName       = "__debug_line__";

        tryCreateBuiltin("__debug_line__", debugLineVertShader_, debugLineFragShader_,
                         debugLinePipeline_, desc);
    }

    // -- Pink error pipeline (constant magenta output) ---------------
    {
        RHIPipelineDesc desc{};
        desc.renderPass      = offscreenPass_;
        desc.vertexAttrCount = 3;
        desc.vertexStride    = sizeof(RHIVertex);
        desc.vertexAttrs[0]  = {0, RHIFormat::RGB32F, 0};
        desc.vertexAttrs[1]  = {1, RHIFormat::RGB32F, 12};
        desc.vertexAttrs[2]  = {2, RHIFormat::RG32F,  24};
        desc.topology        = RHITopology::TriangleList;
        desc.rasterizer      = {RHICullMode::Back, RHIFrontFace::CounterClockwise,
                                RHIPolygonMode::Fill, false};
        desc.depthStencil    = {true, true, RHICompareOp::Less, false};
        desc.debugName       = "__pink_error__";

        tryCreateBuiltin("__pink_error__", pinkVertShader_, pinkFragShader_,
                         pinkErrorPipeline_, desc);
    }
}

void RenderPipeline::DestroyAllPipelines() {
    auto* device = config_.device;
    if (!device) return;

    // Destroy scene pipelines
    for (auto& [name, pipeline] : scenePipelineCache_) {
        if (pipeline.IsValid()) device->DestroyPipeline(pipeline);
    }
    scenePipelineCache_.clear();
    shaderIdMap_.clear();
    nextShaderId_ = 1;

    // Destroy built-in pipelines
    auto destroy = [&](RHIPipeline& p) {
        if (p.IsValid()) { device->DestroyPipeline(p); p = {}; }
    };
    destroy(blitPipeline_);
    destroy(overlayPipeline_);
    destroy(debugLinePipeline_);
    destroy(pinkErrorPipeline_);
}

void RenderPipeline::InvalidatePipelineCache() {
    auto* device = config_.device;
    if (!device) return;

    // Ensure all in-flight GPU work is finished before destroying pipelines.
    // This prevents use-after-free when command buffers still reference them
    // (e.g. during hot-reload).  Acceptable cost for a dev-only operation.
    device->WaitIdle();

    // Clear the MaterialBinder cache first - it holds pipeline handles that
    // are about to be destroyed.  Bindings will be lazily recreated on the
    // next draw via GetOrCreateScenePipeline().
    if (materialBinder_) materialBinder_->Clear();

    for (auto& [name, pipeline] : scenePipelineCache_) {
        if (pipeline.IsValid()) device->DestroyPipeline(pipeline);
    }
    scenePipelineCache_.clear();
}

// -- Rendering helpers --------------------------------------------------

void RenderPipeline::BuildViewProjection(CameraBase* camera,
                                          float* viewOut, float* projOut,
                                          float* cameraPosOut) {
    // Set base rotation from camera layout
    camera->GetTransform()->SetBaseRotation(
        camera->GetCameraLayout()->GetRotation());

    Quaternion lookDir = camera->GetTransform()->GetRotation()
                             .Multiply(camera->GetLookOffset());
    Vector3D camPos = camera->GetTransform()->GetPosition();
    Vector3D forward = lookDir.RotateVector({0, 0, -1}); // -Z forward
    Vector3D up      = lookDir.RotateVector({0, 1, 0});  // +Y up

    // View matrix (LookAt)
    Matrix4x4 viewMat = Matrix4x4::LookAt(camPos, camPos + forward, up);

    // Projection matrix
    Vector2D minC = camera->GetCameraMinCoordinate();
    Vector2D maxC = camera->GetCameraMaxCoordinate();
    float vpW = maxC.X - minC.X + 1.0f;
    float vpH = maxC.Y - minC.Y + 1.0f;
    float aspect = vpW / vpH;

    Matrix4x4 projMat;
    if (camera->IsPerspective()) {
        float fovRad = camera->GetFOV() * kPi / 180.0f;
        projMat = Matrix4x4::Perspective(fovRad, aspect,
                                          camera->GetNearPlane(),
                                          camera->GetFarPlane());
    } else {
        float halfW = vpW * 0.5f;
        float halfH = vpH * 0.5f;
        projMat = Matrix4x4::Orthographic(-halfW, halfW, -halfH, halfH,
                                            camera->GetNearPlane(),
                                            camera->GetFarPlane());
    }

    // Depth range remap: OpenGL [-1,1] -> Vulkan [0,1]
    if (config_.vulkanDepthRemap) {
        for (int c = 0; c < 4; ++c) {
            projMat.M[2][c] = 0.5f * projMat.M[2][c] + 0.5f * projMat.M[3][c];
        }
    }

    // Transpose for GPU upload (row-major -> column-major std140)
    Matrix4x4 viewT = viewMat.Transpose();
    Matrix4x4 projT = projMat.Transpose();

    std::memcpy(viewOut, &viewT.M[0][0], 64);
    std::memcpy(projOut, &projT.M[0][0], 64);

    cameraPosOut[0] = camPos.X;
    cameraPosOut[1] = camPos.Y;
    cameraPosOut[2] = camPos.Z;
    cameraPosOut[3] = 0.0f;
}

Ray RenderPipeline::BuildPickRay(CameraBase* camera, float ndcX, float ndcY) const {
    if (!camera) return Ray(Vector3D(0, 0, 0), Vector3D(0, 0, 0));

    camera->GetTransform()->SetBaseRotation(
        camera->GetCameraLayout()->GetRotation());

    Quaternion lookDir = camera->GetTransform()->GetRotation()
                             .Multiply(camera->GetLookOffset());
    Vector3D camPos  = camera->GetTransform()->GetPosition();
    Vector3D forward = lookDir.RotateVector({0, 0, -1});
    Vector3D up      = lookDir.RotateVector({0, 1, 0});

    Matrix4x4 viewMat = Matrix4x4::LookAt(camPos, camPos + forward, up);

    Vector2D minC = camera->GetCameraMinCoordinate();
    Vector2D maxC = camera->GetCameraMaxCoordinate();
    float vpW = maxC.X - minC.X + 1.0f;
    float vpH = maxC.Y - minC.Y + 1.0f;
    float aspect = vpW / vpH;

    Matrix4x4 projMat;
    if (camera->IsPerspective()) {
        float fovRad = camera->GetFOV() * kPi / 180.0f;
        projMat = Matrix4x4::Perspective(fovRad, aspect,
                                          camera->GetNearPlane(),
                                          camera->GetFarPlane());
    } else {
        float halfW = vpW * 0.5f;
        float halfH = vpH * 0.5f;
        projMat = Matrix4x4::Orthographic(-halfW, halfW, -halfH, halfH,
                                            camera->GetNearPlane(),
                                            camera->GetFarPlane());
    }

    // Unproject in canonical OpenGL clip space ([-1,+1] depth) - no Vulkan
    // depth remap, so the inverse is well-conditioned regardless of backend.
    Matrix4x4 invVP = projMat.Multiply(viewMat).Inverse();

    auto unproject = [&](float z) {
        Vector3D clip(ndcX, ndcY, z);
        // Matrix4x4::Multiply(Vector3D) is assumed to do perspective divide
        // when the matrix has a non-trivial w-row. Fall back to manual w.
        float wx = invVP.M[0][0]*clip.X + invVP.M[0][1]*clip.Y + invVP.M[0][2]*clip.Z + invVP.M[0][3];
        float wy = invVP.M[1][0]*clip.X + invVP.M[1][1]*clip.Y + invVP.M[1][2]*clip.Z + invVP.M[1][3];
        float wz = invVP.M[2][0]*clip.X + invVP.M[2][1]*clip.Y + invVP.M[2][2]*clip.Z + invVP.M[2][3];
        float ww = invVP.M[3][0]*clip.X + invVP.M[3][1]*clip.Y + invVP.M[3][2]*clip.Z + invVP.M[3][3];
        if (ww == 0.0f) ww = 1.0f;
        return Vector3D(wx / ww, wy / ww, wz / ww);
    };

    Vector3D nearPt = unproject(-1.0f);
    Vector3D farPt  = unproject( 1.0f);

    Vector3D origin = camera->IsPerspective() ? camPos : nearPt;
    Vector3D dir    = (farPt - nearPt);
    float    len    = dir.Magnitude();
    if (len > 0.0f) dir = dir / len;

    return Ray(origin, dir);
}

void RenderPipeline::RenderMeshes(Scene* scene,
                                   const float* view, const float* proj,
                                   const float* cameraPos) {
    KL_PERF_SCOPE("RenderPipeline.Meshes");

    auto* device = config_.device;
    unsigned int meshCount = scene->GetMeshCount();
    Mesh**       meshes    = scene->GetMeshes();

    // -- Build draw list with pre-resolved pipelines --------------------
    struct DrawEntry {
        unsigned int meshIndex;
        RHIPipeline  pipeline;
        const MaterialBinding* binding;
        KSLMaterial* kmat;
    };
    static std::vector<DrawEntry> drawList;
    drawList.clear();
    drawList.reserve(meshCount);

    for (unsigned int i = 0; i < meshCount; ++i) {
        Mesh* mesh = meshes[i];
        if (!mesh || !mesh->IsEnabled()) continue;

        ITriangleGroup* triGroup = mesh->GetTriangleGroup();
        if (!triGroup || triGroup->GetTriangleCount() == 0) continue;

        IMaterial* material = mesh->GetMaterial();
        if (!material) continue;

        KSLMaterial* kmat = material->IsKSL()
            ? static_cast<KSLMaterial*>(material) : nullptr;

        RHIPipeline pipeline{};
        const MaterialBinding* binding = nullptr;

        if (kmat && kmat->IsBound() && kmat->GetModule()) {
            binding = materialBinder_->Bind(kmat);
            if (binding && binding->valid) {
                pipeline = binding->pipeline;
            }
        }

        if (!pipeline.IsValid()) {
            pipeline = pinkErrorPipeline_;
        }
        if (!pipeline.IsValid()) continue;

        drawList.push_back({i, pipeline, binding, kmat});
    }

    // -- Sort by (pipeline, material) to minimize state changes ---------
    std::sort(drawList.begin(), drawList.end(),
        [](const DrawEntry& a, const DrawEntry& b) {
            if (a.pipeline.id != b.pipeline.id)
                return a.pipeline.id < b.pipeline.id;
            return reinterpret_cast<uintptr_t>(a.kmat)
                 < reinterpret_cast<uintptr_t>(b.kmat);
        });

    // -- Render sorted draws --------------------------------------------
    RHIPipeline lastPipeline{};
    const KSLMaterial* lastMat = nullptr;

    for (const auto& entry : drawList) {
        // Bind pipeline (skip if unchanged)
        if (entry.pipeline != lastPipeline) {
            device->BindPipeline(entry.pipeline);
            lastPipeline = entry.pipeline;
        }

        // Bind material UBO + texture (skip if same material as previous)
        if (entry.kmat != lastMat) {
            if (entry.binding && entry.binding->valid) {
                device->BindUniformBuffer(entry.binding->uniformBuffer, 1, 0,
                                          0, entry.binding->uniformSize);
            }

            if (entry.kmat && entry.kmat->TextureCount() > 0) {
                Texture* tex = entry.kmat->GetTexture(0);
                if (tex) {
                    RHITexture gpuTex = textureCache_->GetOrUpload(tex);
                    if (gpuTex.IsValid()) {
                        device->BindTexture(gpuTex, 2, 0);
                    }
                }
            }
            lastMat = entry.kmat;
        }

        // Upload mesh and draw
        Mesh* mesh = meshes[entry.meshIndex];
        const CachedMesh* cached = meshCache_->GetOrUpload(mesh);
        if (cached && cached->vertexBuffer.IsValid()) {
            device->BindVertexBuffer(cached->vertexBuffer);
            device->Draw(cached->vertexCount);
        }
    }
}

// Number of segments per great-circle ring used when expanding a DebugSphere
// into line geometry for the Vulkan pipeline.  Each sphere becomes 3 rings
// (XY/XZ/YZ) with this many segments each (kSphereSegments * 6 verts total).
static constexpr int kSphereSegments = 24;

void RenderPipeline::AppendDebugShapes(koilo::DebugDraw& dd, bool depthTestPass,
                                       size_t& depthTestedCount) {
    auto pushVert = [&](float x, float y, float z, const koilo::Color& c) {
        debugLineScratch_.push_back({x, y, z, c.r, c.g, c.b, c.a});
    };
    auto pushSegment = [&](const Vector3D& a, const Vector3D& b,
                           const koilo::Color& c) {
        pushVert(a.X, a.Y, a.Z, c);
        pushVert(b.X, b.Y, b.Z, c);
        if (depthTestPass) depthTestedCount += 2;
    };

    // -- Spheres: 3 great-circle rings (XY, XZ, YZ planes) ----------------
    for (const auto& s : dd.GetSpheres()) {
        if (s.depthTest != depthTestPass) continue;
        const float r = s.radius;
        const Vector3D& C = s.center;

        Vector3D prevXY = C + Vector3D(r, 0.0f, 0.0f);
        Vector3D prevXZ = C + Vector3D(r, 0.0f, 0.0f);
        Vector3D prevYZ = C + Vector3D(0.0f, r, 0.0f);
        for (int i = 1; i <= kSphereSegments; ++i) {
            const float t  = (static_cast<float>(i) /
                              static_cast<float>(kSphereSegments)) * 6.2831853f;
            const float ct = std::cos(t);
            const float st = std::sin(t);

            const Vector3D nxtXY = C + Vector3D(r * ct, r * st, 0.0f);
            const Vector3D nxtXZ = C + Vector3D(r * ct, 0.0f, r * st);
            const Vector3D nxtYZ = C + Vector3D(0.0f, r * ct, r * st);

            pushSegment(prevXY, nxtXY, s.color);
            pushSegment(prevXZ, nxtXZ, s.color);
            pushSegment(prevYZ, nxtYZ, s.color);

            prevXY = nxtXY;
            prevXZ = nxtXZ;
            prevYZ = nxtYZ;
        }
    }

    // -- Boxes: 12 edges (oriented via box.transform if non-identity) ----
    static const int kEdges[12][2] = {
        {0,1},{1,3},{3,2},{2,0},  // -Z face
        {4,5},{5,7},{7,6},{6,4},  // +Z face
        {0,4},{1,5},{2,6},{3,7}   // connecting edges
    };
    for (const auto& b : dd.GetBoxes()) {
        if (b.depthTest != depthTestPass) continue;
        const Vector3D& e = b.extents;
        Vector3D corners[8];
        int idx = 0;
        for (int sx = -1; sx <= 1; sx += 2) {
            for (int sy = -1; sy <= 1; sy += 2) {
                for (int sz = -1; sz <= 1; sz += 2) {
                    Vector3D local(e.X * sx, e.Y * sy, e.Z * sz);
                    // box.transform may carry rotation; translation comes
                    // from box.center (rotation-only convention used by
                    // DrawOrientedBox in the script demo).
                    Vector3D rotated = b.transform.TransformDirection(local);
                    corners[idx++] = b.center + rotated;
                }
            }
        }
        for (int i = 0; i < 12; ++i) {
            pushSegment(corners[kEdges[i][0]], corners[kEdges[i][1]], b.color);
        }
    }
}

void RenderPipeline::RenderDebugLines(const float* view, const float* proj) {
    (void)view;
    (void)proj;
    KL_PERF_SCOPE("RenderPipeline.DebugLines");

    auto& dd = DebugDraw::GetInstance();
    if (!dd.IsEnabled()) return;

    const auto& lines   = dd.GetLines();
    const auto& spheres = dd.GetSpheres();
    const auto& boxes   = dd.GetBoxes();
    if ((lines.empty() && spheres.empty() && boxes.empty())
        || !debugLinePipeline_.IsValid() || !debugLineVBO_.IsValid())
        return;

    auto* device = config_.device;

    const uint64_t version = dd.GetLinesVersion();
    const bool unchanged = (version == debugLinesVersionCached_)
                           && (debugLineUploadCount_ > 0);

    size_t uploadCount    = debugLineUploadCount_;
    size_t depthTestedCount = debugLineDepthCount_;

    if (!unchanged) {
        // Rebuild into the persistent scratch (reuses backing storage).
        debugLineScratch_.clear();
        debugLineScratch_.reserve(lines.size() * 2
                                  + spheres.size() * kSphereSegments * 6
                                  + boxes.size() * 24);

        depthTestedCount = 0;
        // Pass 1: depth-tested.
        for (const auto& line : lines) {
            if (line.depthTest) {
                debugLineScratch_.push_back({line.start.X, line.start.Y, line.start.Z,
                                 line.color.r, line.color.g, line.color.b, line.color.a});
                debugLineScratch_.push_back({line.end.X, line.end.Y, line.end.Z,
                                 line.color.r, line.color.g, line.color.b, line.color.a});
                depthTestedCount += 2;
            }
        }
        AppendDebugShapes(dd, /*depthTestPass=*/true, depthTestedCount);

        // Pass 2: non-depth (drawn after, on top of everything).
        for (const auto& line : lines) {
            if (!line.depthTest) {
                debugLineScratch_.push_back({line.start.X, line.start.Y, line.start.Z,
                                 line.color.r, line.color.g, line.color.b, line.color.a});
                debugLineScratch_.push_back({line.end.X, line.end.Y, line.end.Z,
                                 line.color.r, line.color.g, line.color.b, line.color.a});
            }
        }
        AppendDebugShapes(dd, /*depthTestPass=*/false, depthTestedCount);

        if (debugLineScratch_.empty()) {
            debugLineUploadCount_   = 0;
            debugLineDepthCount_    = 0;
            debugLinesVersionCached_ = version;
            return;
        }

        uploadCount = std::min(debugLineScratch_.size(), kMaxDebugLineVerts);
        const size_t uploadSize = uploadCount * sizeof(DebugLineVertex);
        device->UpdateBuffer(debugLineVBO_, debugLineScratch_.data(), uploadSize);

        debugLineUploadCount_    = uploadCount;
        debugLineDepthCount_     = depthTestedCount;
        debugLinesVersionCached_ = version;
    }

    if (uploadCount == 0) return;

    // Bind the transform UBO populated by BeginScenePass.  view/proj
    // already match what the debug-line shader needs (model = identity);
    // the prior implementation re-uploaded an identical buffer here every
    // frame, which is now elided (B1).
    device->BindUniformBuffer(transformUBO_, 0, 0);

    device->BindPipeline(debugLinePipeline_);
    device->BindVertexBuffer(debugLineVBO_);

    // Draw depth-tested lines
    if (depthTestedCount > 0 && depthTestedCount <= uploadCount) {
        device->Draw(static_cast<uint32_t>(depthTestedCount));
    }

    // Draw non-depth-tested lines (drawn after, on top of everything)
    size_t noDepthCount = uploadCount - std::min(depthTestedCount, uploadCount);
    if (noDepthCount > 0) {
        // These reuse the depth-tested pipeline; a no-depth-test pipeline
        // variant could be used to actually disable depth testing for them.
        device->Draw(static_cast<uint32_t>(noDepthCount),
                     1,
                     static_cast<uint32_t>(depthTestedCount));
    }
}

void RenderPipeline::UploadLights(Scene* scene) {
    KL_PERF_SCOPE("RenderPipeline.Lights");

    auto* device = config_.device;

    // Collect lights from the first KSL material that has lights
    // (matching existing Vulkan backend behavior)
    int lightCount = 0;
    GPULight gpuLights[kMaxGPULights] = {};

    unsigned int meshCount = scene->GetMeshCount();
    Mesh** meshes = scene->GetMeshes();
    for (unsigned int i = 0; i < meshCount && lightCount == 0; ++i) {
        if (!meshes[i] || !meshes[i]->GetMaterial()) continue;
        IMaterial* mat = meshes[i]->GetMaterial();
        if (!mat->IsKSL()) continue;

        auto* kmat = static_cast<KSLMaterial*>(mat);
        const auto& lights = kmat->GetLights();
        lightCount = static_cast<int>(
            std::min(lights.size(), static_cast<size_t>(kMaxGPULights)));

        for (int li = 0; li < lightCount; ++li) {
            const auto& ld = lights[li];
            gpuLights[li].position[0] = ld.position.x;
            gpuLights[li].position[1] = ld.position.y;
            gpuLights[li].position[2] = ld.position.z;
            gpuLights[li].intensity   = ld.intensity;
            gpuLights[li].color[0]    = ld.color.x;
            gpuLights[li].color[1]    = ld.color.y;
            gpuLights[li].color[2]    = ld.color.z;
            gpuLights[li].falloff     = ld.falloff;
            gpuLights[li].curve       = ld.curve;
        }
    }

    // -- Light upload dirty check (B2) --------------------------------
    // The light data rarely changes every frame.  Compare against the
    // previously-uploaded snapshot and skip the SSBO + scene UBO writes
    // when nothing has changed.  Bindings still happen unconditionally
    // because the descriptor set must be valid for every draw.
    const size_t lightBytes = sizeof(GPULight) * static_cast<size_t>(lightCount);
    bool lightsChanged = (lightCount != lastUploadedLightCount_);
    if (!lightsChanged && lightCount > 0) {
        lightsChanged = (std::memcmp(gpuLights, lastUploadedLights_,
                                     lightBytes) != 0);
    }

    if (lightsChanged) {
        // Upload scene UBO (light count) -- binding 2 matches SPIR-V layout
        SceneUBO sceneData{};
        sceneData.lightCount = lightCount;
        device->UpdateBuffer(sceneUBO_, &sceneData, sizeof(sceneData));

        // Upload light data -- binding 1 matches SPIR-V layout (LightBuffer SSBO)
        if (lightCount > 0) {
            device->UpdateBuffer(lightSSBO_, gpuLights, lightBytes);
            std::memcpy(lastUploadedLights_, gpuLights, lightBytes);
        }
        lastUploadedLightCount_ = lightCount;
    }

    device->BindUniformBuffer(sceneUBO_, 0, 2);
    // Always bind even if empty -- all 4 set-0 descriptors must be valid
    device->BindUniformBuffer(lightSSBO_, 0, 1, 0,
                              sizeof(GPULight) * kMaxGPULights);

    // Binding 3: audio SSBO (dummy, required by scene set layout)
    device->BindUniformBuffer(audioSSBO_, 0, 3);
}

void RenderPipeline::DrawFullscreenQuad(RHIPipeline pipeline,
                                         RHITexture texture,
                                         float alpha) {
    if (!pipeline.IsValid() || !fullscreenQuadVBO_.IsValid()) return;

    auto* device = config_.device;

    device->BindPipeline(pipeline);
    device->BindVertexBuffer(fullscreenQuadVBO_);

    if (texture.IsValid()) {
        device->BindTexture(texture, 0, 0);
    }

    // Upload alpha uniform for overlay blending
    if (alpha < 1.0f) {
        float alphaData[4] = {alpha, 0.0f, 0.0f, 0.0f};
        device->UpdateBuffer(overlayUBO_, alphaData, sizeof(alphaData));
        device->BindUniformBuffer(overlayUBO_, 1, 0);
    }

    device->Draw(6); // 2 triangles
}

} // namespace koilo::rhi
