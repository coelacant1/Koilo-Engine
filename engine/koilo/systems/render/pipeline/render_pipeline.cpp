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
    destroyBuffer(sceneUBO_);
    destroyBuffer(lightSSBO_);
    destroyBuffer(audioSSBO_);
    destroyBuffer(overlayUBO_);

    KL_LOG("RenderPipeline", "Shutdown: destroying offscreen target...");
    // Destroy off-screen target
    DestroyOffscreenTarget();

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

bool RenderPipeline::IsVulkanDevice() const {
    if (!config_.device) return false;
    const char* name = config_.device->GetName();
    return name && std::string(name).find("Vulkan") != std::string::npos;
}

// -- IGPURenderBackend: PrepareFrame / FinishFrame ----------------------

void RenderPipeline::PrepareFrame() {
    if (!initialized_) return;
    config_.device->BeginFrame();
}

void RenderPipeline::FinishFrame() {
    if (!initialized_) return;
    config_.device->EndFrame();
    config_.device->Present();
}

// -- IRenderBackend: Render (single-call convenience) -------------------

void RenderPipeline::Render(Scene* scene, CameraBase* camera) {
    RenderDirect(scene, camera);
}

void RenderPipeline::ReadPixels(Color888* /*buffer*/, int /*width*/, int /*height*/) {
    // Readback not yet implemented via RHI -- requires staging buffer support.
}

// -- IGPURenderBackend: RenderDirect ------------------------------------

void RenderPipeline::RenderDirect(Scene* scene, CameraBase* camera) {
    if (!BeginScenePass(scene, camera))
        return;
    RenderSky();
    RenderSceneMeshes();
    RenderSceneDebugLines();
    EndScenePass();
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

    // Identity transform for sky (screen-space quad)
    TransformUBO skyUbo{};
    for (int i = 0; i < 4; ++i) {
        skyUbo.model[i * 4 + i]      = 1.0f;
        skyUbo.view[i * 4 + i]       = 1.0f;
        skyUbo.projection[i * 4 + i] = 1.0f;
    }
    std::memcpy(skyUbo.inverseViewProj, sceneInvVP_,
                sizeof(skyUbo.inverseViewProj));
    device->UpdateBuffer(transformUBO_, &skyUbo, sizeof(skyUbo));
    device->BindUniformBuffer(transformUBO_, 0, 0);

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

    // Restore camera transform for subsequent draws
    TransformUBO camUbo{};
    for (int i = 0; i < 4; ++i) camUbo.model[i * 4 + i] = 1.0f;
    std::memcpy(camUbo.view, sceneView_, sizeof(camUbo.view));
    std::memcpy(camUbo.projection, sceneProj_, sizeof(camUbo.projection));
    std::memcpy(camUbo.cameraPos, sceneCameraPos_, sizeof(camUbo.cameraPos));
    std::memcpy(camUbo.inverseViewProj, sceneInvVP_,
                sizeof(camUbo.inverseViewProj));
    device->UpdateBuffer(transformUBO_, &camUbo, sizeof(camUbo));
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

void RenderPipeline::CompositeCanvasOverlays(int screenW, int screenH) {
    auto& canvases = Canvas2D::ActiveList();
    if (canvases.empty() || !initialized_) return;
    if (!overlayPipeline_.IsValid() || !fullscreenQuadVBO_.IsValid()) return;

    auto* device = config_.device;

    for (auto* canvas : canvases) {
        if (!canvas) continue;

        int cw = canvas->GetWidth();
        int ch = canvas->GetHeight();
        if (cw <= 0 || ch <= 0) continue;

        const Color888* pixels = canvas->GetPixels();
        const uint8_t*  alpha  = canvas->GetAlpha();
        if (!pixels || !alpha) continue;

        // Convert RGB + separate alpha -> RGBA for GPU upload
        size_t pixelCount = static_cast<size_t>(cw) * ch;
        std::vector<uint8_t> rgba(pixelCount * 4);
        for (size_t i = 0; i < pixelCount; ++i) {
            rgba[i * 4 + 0] = pixels[i].r;
            rgba[i * 4 + 1] = pixels[i].g;
            rgba[i * 4 + 2] = pixels[i].b;
            rgba[i * 4 + 3] = alpha[i];
        }

        // Create or reuse a temporary texture for this canvas
        RHITextureDesc texDesc{};
        texDesc.width  = static_cast<uint32_t>(cw);
        texDesc.height = static_cast<uint32_t>(ch);
        texDesc.format = RHIFormat::RGBA8_Unorm;
        texDesc.usage  = RHITextureUsage::Sampled | RHITextureUsage::TransferDst;
        texDesc.filter = RHISamplerFilter::Nearest;
        texDesc.debugName = "Pipeline.CanvasOverlay";

        RHITexture canvasTexture = device->CreateTexture(texDesc);
        if (!canvasTexture.IsValid()) continue;

        device->UpdateTexture(canvasTexture, rgba.data(), rgba.size(),
                              texDesc.width, texDesc.height);

        DrawFullscreenQuad(overlayPipeline_, canvasTexture, 1.0f);

        // Destroy the temporary texture (per-frame upload)
        device->DestroyTexture(canvasTexture);
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
    // Build a cache key that includes render flag overrides
    std::string cacheKey = shaderName;
    if (flags) {
        cacheKey += "|d";
        cacheKey += (flags->depthStencil.depthTest  ? '1' : '0');
        cacheKey += (flags->depthStencil.depthWrite ? '1' : '0');
        cacheKey += '|';
        cacheKey += static_cast<char>('0' + static_cast<int>(flags->cullMode));
    }

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

    // Destroy built-in pipelines
    auto destroy = [&](RHIPipeline& p) {
        if (p.IsValid()) { device->DestroyPipeline(p); p = {}; }
    };
    destroy(blitPipeline_);
    destroy(overlayPipeline_);
    destroy(debugLinePipeline_);
    destroy(pinkErrorPipeline_);
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

void RenderPipeline::RenderDebugLines(const float* view, const float* proj) {
    KL_PERF_SCOPE("RenderPipeline.DebugLines");

    auto& dd = DebugDraw::GetInstance();
    if (!dd.IsEnabled()) return;

    const auto& lines = dd.GetLines();
    if (lines.empty() || !debugLinePipeline_.IsValid() || !debugLineVBO_.IsValid())
        return;

    auto* device = config_.device;

    // Build vertex data: depth-tested lines first, then non-depth-tested
    std::vector<DebugLineVertex> verts;
    verts.reserve(lines.size() * 2);

    size_t depthTestedCount = 0;
    for (const auto& line : lines) {
        if (line.depthTest) {
            verts.push_back({line.start.X, line.start.Y, line.start.Z,
                             line.color.r, line.color.g, line.color.b, line.color.a});
            verts.push_back({line.end.X, line.end.Y, line.end.Z,
                             line.color.r, line.color.g, line.color.b, line.color.a});
            depthTestedCount += 2;
        }
    }

    // Non-depth-tested lines (TODO: use separate no-depth pipeline)
    for (const auto& line : lines) {
        if (!line.depthTest) {
            verts.push_back({line.start.X, line.start.Y, line.start.Z,
                             line.color.r, line.color.g, line.color.b, line.color.a});
            verts.push_back({line.end.X, line.end.Y, line.end.Z,
                             line.color.r, line.color.g, line.color.b, line.color.a});
        }
    }

    if (verts.empty()) return;

    // Clamp to max buffer size
    size_t uploadCount = std::min(verts.size(), kMaxDebugLineVerts);
    size_t uploadSize  = uploadCount * sizeof(DebugLineVertex);
    device->UpdateBuffer(debugLineVBO_, verts.data(), uploadSize);

    // Upload view/projection for line shader via transform UBO
    // (reuse transform UBO with identity model)
    {
        TransformUBO ubo{};
        for (int i = 0; i < 4; ++i) ubo.model[i * 4 + i] = 1.0f;
        std::memcpy(ubo.view, view, 64);
        std::memcpy(ubo.projection, proj, 64);
        device->UpdateBuffer(transformUBO_, &ubo, sizeof(ubo));
        device->BindUniformBuffer(transformUBO_, 0, 0);
    }

    device->BindPipeline(debugLinePipeline_);
    device->BindVertexBuffer(debugLineVBO_);

    // Draw depth-tested lines
    if (depthTestedCount > 0 && depthTestedCount <= uploadCount) {
        device->Draw(static_cast<uint32_t>(depthTestedCount));
    }

    // Draw non-depth-tested lines (drawn after, on top of everything)
    size_t noDepthCount = uploadCount - std::min(depthTestedCount, uploadCount);
    if (noDepthCount > 0) {
        // TODO: In 17g, ideally switch to a no-depth-test pipeline variant
        // for the second draw call. For now, all lines use the same pipeline.
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

    // Upload scene UBO (light count) -- binding 2 matches SPIR-V layout
    SceneUBO sceneData{};
    sceneData.lightCount = lightCount;
    device->UpdateBuffer(sceneUBO_, &sceneData, sizeof(sceneData));
    device->BindUniformBuffer(sceneUBO_, 0, 2);

    // Upload light data -- binding 1 matches SPIR-V layout (LightBuffer SSBO)
    // Always bind even if empty -- all 4 set-0 descriptors must be valid
    if (lightCount > 0) {
        device->UpdateBuffer(lightSSBO_, gpuLights,
                             sizeof(GPULight) * lightCount);
    }
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
