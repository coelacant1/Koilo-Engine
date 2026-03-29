// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file render_pipeline.hpp
 * @brief Unified GPU render pipeline using the RHI abstraction layer.
 *
 * Implements IGPURenderBackend by delegating all GPU operations through
 * IRHIDevice.  Owns MeshCache, TextureCache, and MaterialBinder for
 * shared resource management.  Replaces the duplicated scene traversal,
 * sky rendering, debug line, overlay compositing, and blit logic that
 * previously lived in both the OpenGL and Vulkan render backends.
 *
 * The pipeline is backend-agnostic - it works with any IRHIDevice
 * implementation (Vulkan, OpenGL, or future backends).
 *
 * @date 03/19/2026
 * @author Coela Can't
 */
#pragma once

#include "../igpu_render_backend.hpp"
#include "../rhi/rhi_types.hpp"
#include "mesh_cache.hpp"
#include "texture_cache.hpp"
#include "material_binder.hpp"
#include "../../../registry/reflect_macros.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace koilo      { class Scene; class CameraBase; class KSLMaterial; }
namespace koilo::rhi { class IRHIDevice; }
namespace ksl        { class KSLRegistry; }

namespace koilo::rhi {

// -- Shader data resolved by the factory for a given KSL shader name ----
struct ShaderData {
    const void* vertexCode     = nullptr;
    size_t      vertexCodeSize = 0;
    const void* fragCode       = nullptr;
    size_t      fragCodeSize   = 0;
};

/// Callback that resolves shader bytecode for a KSL shader name.
/// For Vulkan: provides SPIR-V. For OpenGL: provides GLSL source.
using ShaderResolver = std::function<ShaderData(const std::string& name)>;

// -- Configuration ------------------------------------------------------

struct RenderPipelineConfig {
    IRHIDevice*       device        = nullptr;
    ksl::KSLRegistry* shaderRegistry = nullptr;
    ShaderResolver    shaderResolver;

    /// Apply Vulkan depth range remap: [-1,1] -> [0,1] on projection matrix.
    bool vulkanDepthRemap = false;

    /// Initial off-screen render target dimensions (resized dynamically).
    uint32_t initialWidth  = 1920;
    uint32_t initialHeight = 1080;

    KL_BEGIN_FIELDS(RenderPipelineConfig)
        KL_FIELD(RenderPipelineConfig, initialWidth, "Initial width", 0, 4294967295),
        KL_FIELD(RenderPipelineConfig, initialHeight, "Initial height", 0, 4294967295)
    KL_END_FIELDS

    KL_BEGIN_METHODS(RenderPipelineConfig)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(RenderPipelineConfig)
        /* No reflected ctors. */
    KL_END_DESCRIBE(RenderPipelineConfig)

};

// -- GPU UBO layouts (std140-compatible) --------------------------------

struct alignas(16) TransformUBO {
    float model[16];
    float view[16];
    float projection[16];
    float cameraPos[4]; // xyz + pad
    float inverseViewProj[16]; // for shaders that reconstruct view direction
};

struct alignas(16) SceneUBO {
    int   lightCount;
    int   pad[3];
};

struct alignas(16) GPULight {
    float position[3];
    float intensity;
    float color[3];
    float falloff;
    float curve;
    float pad[3];
};

static constexpr int kMaxGPULights = 16;

// -- Debug line vertex (position + color) -------------------------------

struct DebugLineVertex {
    float px, py, pz;
    float r, g, b, a;

    KL_BEGIN_FIELDS(DebugLineVertex)
        KL_FIELD(DebugLineVertex, pz, "Pz", 0, 0),
        KL_FIELD(DebugLineVertex, a, "A", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(DebugLineVertex)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(DebugLineVertex)
        /* No reflected ctors. */
    KL_END_DESCRIBE(DebugLineVertex)

};

// -- Fullscreen quad vertex (position + UV) -----------------------------

struct FullscreenVertex {
    float px, py;
    float u, v;

    KL_BEGIN_FIELDS(FullscreenVertex)
        KL_FIELD(FullscreenVertex, py, "Py", 0, 0),
        KL_FIELD(FullscreenVertex, v, "V", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(FullscreenVertex)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(FullscreenVertex)
        /* No reflected ctors. */
    KL_END_DESCRIBE(FullscreenVertex)

};

// -- RenderPipeline -----------------------------------------------------

/**
 * @class RenderPipeline
 * @brief Unified GPU render pipeline implementing IGPURenderBackend.
 *
 * Owns all shared GPU caches and pipeline state.  Scene traversal,
 * sky rendering, debug lines, and overlay compositing are written once
 * here instead of being duplicated across backend implementations.
 */
class RenderPipeline : public IGPURenderBackend {
public:
    RenderPipeline();
    ~RenderPipeline() override;

    RenderPipeline(const RenderPipeline&)            = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    /// Configure the pipeline. Must be called before Initialize().
    void Configure(const RenderPipelineConfig& config);

    /// Transfer ownership of the IRHIDevice to this pipeline.
    /// The device will be destroyed on Shutdown().
    void OwnDevice(std::unique_ptr<IRHIDevice> device);

    /// Transfer ownership of a KSLRegistry to this pipeline.
    /// The registry will be destroyed on Shutdown().
    void OwnRegistry(std::unique_ptr<ksl::KSLRegistry> registry);

    // -- IRenderBackend ------------------------------------------------
    bool        Initialize() override;
    void        Shutdown() override;
    bool        IsInitialized() const override;
    void        Render(Scene* scene, CameraBase* camera) override;
    void        ReadPixels(Color888* buffer, int width, int height) override;
    const char* GetName() const override;

    // -- IGPURenderBackend ---------------------------------------------
    void RenderDirect(Scene* scene, CameraBase* camera) override;
    void BlitToScreen(int screenW, int screenH) override;
    void CompositeCanvasOverlays(int screenW, int screenH) override;
    void PrepareFrame() override;
    void FinishFrame() override;

    // -- Granular render methods (for render graph integration) --------

    /// Begin the offscreen scene pass: setup viewport, upload matrices
    /// and lights.  Returns false if the pass cannot start.
    bool BeginScenePass(Scene* scene, CameraBase* camera);

    /// Render the sky (requires an active scene pass).
    void RenderSky();

    /// Render all scene meshes (requires an active scene pass).
    void RenderSceneMeshes();

    /// Render debug lines (requires an active scene pass).
    void RenderSceneDebugLines();

    /// End the offscreen scene pass.
    void EndScenePass();

    /// Access the underlying IRHIDevice (for UI rendering etc.).
    rhi::IRHIDevice* GetDevice() const { return config_.device; }

    /// Whether the device is Vulkan-based.
    bool IsVulkanDevice() const;

private:
    // -- Off-screen render target management ---------------------------

    void EnsureOffscreenTarget(uint32_t width, uint32_t height);
    void DestroyOffscreenTarget();

    // -- Pipeline creation / caching ----------------------------------

    RHIPipeline GetOrCreateScenePipeline(const std::string& shaderName,
                                         const RenderFlags* flags = nullptr);
    void        CreateBuiltinPipelines();
    void        DestroyAllPipelines();

    // -- Rendering helpers --------------------------------------------

    void BuildViewProjection(CameraBase* camera,
                             float* viewOut, float* projOut,
                             float* cameraPosOut);
    void RenderMeshes(Scene* scene,
                      const float* view, const float* proj,
                      const float* cameraPos);
    void RenderDebugLines(const float* view, const float* proj);
    void UploadLights(Scene* scene);
    void DrawFullscreenQuad(RHIPipeline pipeline, RHITexture texture, float alpha = 1.0f);

    // -- State --------------------------------------------------------

    RenderPipelineConfig config_;
    bool                 initialized_ = false;

    // Shared caches
    std::unique_ptr<MeshCache>      meshCache_;
    std::unique_ptr<TextureCache>   textureCache_;
    std::unique_ptr<MaterialBinder> materialBinder_;

    // Optionally owned device + registry (factory transfers ownership)
    std::unique_ptr<IRHIDevice>      ownedDevice_;
    std::unique_ptr<ksl::KSLRegistry> ownedRegistry_;

    // Off-screen render target
    RHITexture      offscreenColor_     = {};
    RHITexture      offscreenDepth_     = {};
    RHIRenderPass   offscreenPass_      = {};
    RHIFramebuffer  offscreenFB_        = {};
    uint32_t        offscreenWidth_     = 0;
    uint32_t        offscreenHeight_    = 0;

    // Built-in pipelines
    RHIPipeline blitPipeline_       = {};
    RHIPipeline overlayPipeline_    = {};
    RHIPipeline debugLinePipeline_  = {};
    RHIPipeline pinkErrorPipeline_  = {};

    // Built-in shaders
    RHIShader blitVertShader_      = {};
    RHIShader blitFragShader_      = {};
    RHIShader overlayFragShader_   = {};
    RHIShader debugLineVertShader_ = {};
    RHIShader debugLineFragShader_ = {};
    RHIShader pinkVertShader_      = {};
    RHIShader pinkFragShader_      = {};

    // Geometry buffers
    RHIBuffer fullscreenQuadVBO_   = {};  // FullscreenVertex format (blit/overlay)
    RHIBuffer sceneQuadVBO_        = {};  // RHIVertex format (sky/effects via scene pipeline)
    RHIBuffer debugLineVBO_        = {};

    // Uniform buffers
    RHIBuffer transformUBO_        = {};
    RHIBuffer sceneUBO_            = {};
    RHIBuffer lightSSBO_           = {};
    RHIBuffer audioSSBO_           = {};
    RHIBuffer overlayUBO_          = {};

    // Scene pipeline cache: shader name -> compiled pipeline
    std::unordered_map<std::string, RHIPipeline> scenePipelineCache_;

    // Render pass for blit / overlay (renders to swapchain)
    RHIRenderPass   swapchainPass_  = {};

    // Per-frame scene pass state (set by BeginScenePass, used by granular methods)
    Scene*   currentScene_      = nullptr;
    float    sceneView_[16]     = {};
    float    sceneProj_[16]     = {};
    float    sceneCameraPos_[4] = {};
    float    sceneInvVP_[16]    = {};
    bool     scenePassActive_   = false;

    KL_BEGIN_FIELDS(RenderPipeline)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(RenderPipeline)
        KL_METHOD_AUTO(RenderPipeline, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(RenderPipeline, IsInitialized, "Is initialized"),
        KL_METHOD_AUTO(RenderPipeline, Render, "Render"),
        KL_METHOD_AUTO(RenderPipeline, ReadPixels, "Read pixels"),
        KL_METHOD_AUTO(RenderPipeline, GetName, "Get name"),
        KL_METHOD_AUTO(RenderPipeline, BlitToScreen, "Blit to screen"),
        KL_METHOD_AUTO(RenderPipeline, CompositeCanvasOverlays, "Composite canvas overlays"),
        KL_METHOD_AUTO(RenderPipeline, PrepareFrame, "Prepare frame"),
        KL_METHOD_AUTO(RenderPipeline, FinishFrame, "Finish frame")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(RenderPipeline)
        KL_CTOR0(RenderPipeline)
    KL_END_DESCRIBE(RenderPipeline)

};

} // namespace koilo::rhi
