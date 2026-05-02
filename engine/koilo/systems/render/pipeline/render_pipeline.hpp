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
#include "../../../core/geometry/ray.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace koilo      { class Scene; class CameraBase; class KSLMaterial; class Canvas2D; class DebugDraw; }
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
    IRHIDevice*         device        = nullptr;
    ::ksl::KSLRegistry* shaderRegistry = nullptr;
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
class RenderPipeline final : public IGPURenderBackend {
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
    void OwnRegistry(std::unique_ptr<::ksl::KSLRegistry> registry);

    // -- IRenderBackend ------------------------------------------------
    bool        Initialize() override;
    void        Shutdown() override;
    bool        IsInitialized() const override;
    void        Render(Scene* scene, CameraBase* camera) override;
    void        ReadPixels(Color888* buffer, int width, int height) override;
    const char* GetName() const override;

    // -- IGPURenderBackend ---------------------------------------------
    void BlitToScreen(int screenW, int screenH) override;
    void CompositeCanvasOverlays(int screenW, int screenH) override;
    void PrepareFrame() override;
    void FinishFrame() override;

    /// Stage canvas overlay texture uploads via the active frame command
    /// buffer.  MUST be called after PrepareFrame() / BeginFrame() but
    /// BEFORE any render pass is opened (vkCmdCopyBufferToImage is forbidden
    /// inside a render pass).  When this is called, a subsequent
    /// CompositeCanvasOverlays() in the same frame skips its own upload
    /// step and only issues fullscreen-quad draws.
    ///
    /// Backends without proper frame-command-buffer staging fall back to
    /// the legacy synchronous path (see IRHIDevice::StageTextureFull).
    void StageCanvasOverlays(int screenW, int screenH);

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

    /// Whether the device uses top-left screen origin (Vulkan, Software).
    bool UsesTopLeftOrigin() const;

    /// Access the owned KSL registry (for hot-reload).
    ::ksl::KSLRegistry* GetRegistry() const { return ownedRegistry_.get(); }

    /// Invalidate the scene pipeline cache.  Cached pipelines are destroyed
    /// and will be lazily recreated on the next draw using the current
    /// shader sources from the registry.
    void InvalidatePipelineCache();

    /// Underlying RHI handle id of the offscreen color render target
    /// (the texture the scene is rendered into before BlitToScreen).
    /// Returns 0 before Initialize() or after Shutdown().  The id can
    /// change when the target is recreated on resize -- callers should
    /// re-read it each frame instead of caching long term.
    /// Used by the editor viewport widget to display the live scene.
    std::uint32_t GetOffscreenColorTextureId() const {
        return offscreenColor_.id;
    }
    std::uint32_t GetOffscreenWidth()  const { return offscreenWidth_;  }
    std::uint32_t GetOffscreenHeight() const { return offscreenHeight_; }

    /// Build a world-space picking ray from a point in normalized device
    /// coordinates ([-1,+1] X right, [-1,+1] Y up) using the same camera
    /// math the renderer uses (LookAt + Perspective/Ortho), but WITHOUT
    /// the Vulkan depth remap so unprojection works in canonical clip
    /// space.  Returns a zero-direction ray if camera is null.
    Ray BuildPickRay(CameraBase* camera, float ndcX, float ndcY) const;

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
    // Expand DebugDraw boxes (12 edges each) and spheres (3 great circles)
    // into line vertices appended to debugLineScratch_. depthTestPass selects
    // which subset to emit; depthTestedCount is incremented when emitting
    // depth-tested verts so the draw split stays correct.
    void AppendDebugShapes(koilo::DebugDraw& dd, bool depthTestPass,
                           size_t& depthTestedCount);
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
    std::unique_ptr<::ksl::KSLRegistry> ownedRegistry_;

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

    // -- DebugLines upload cache (P6) ----------------------------------
    // Skip the per-frame vertex rebuild + UpdateBuffer when the source
    // line set hasn't changed.  Tracks the DebugDraw version that was
    // last uploaded plus the split between depth-tested / no-depth lines.
    std::vector<DebugLineVertex> debugLineScratch_;
    uint64_t debugLinesVersionCached_ = 0;
    size_t   debugLineDepthCount_     = 0;
    size_t   debugLineUploadCount_    = 0;

    // Uniform buffers
    RHIBuffer transformUBO_        = {};
    RHIBuffer skyTransformUBO_     = {};  // dedicated UBO for sky pass (B1)
    RHIBuffer sceneUBO_            = {};
    RHIBuffer lightSSBO_           = {};
    RHIBuffer audioSSBO_           = {};
    RHIBuffer overlayUBO_          = {};

    // -- Light upload dirty tracking (B2) ------------------------------
    // A snapshot of the last gpuLights / lightCount uploaded.  Lights
    // typically don't change every frame, so we skip the SSBO + scene
    // UBO uploads if the contents would be byte-identical.
    GPULight lastUploadedLights_[kMaxGPULights] = {};
    int      lastUploadedLightCount_            = -1;

    // Scene pipeline cache: numeric (shaderId, flags) key -> compiled pipeline (B5)
    // shaderId is interned at resolve time to avoid std::string allocation/hashing
    // on the per-frame lookup path.
    struct PipelineKey {
        uint32_t shaderId;
        uint16_t flags;
        bool operator==(const PipelineKey& o) const noexcept {
            return shaderId == o.shaderId && flags == o.flags;
        }
    };
    struct PipelineKeyHash {
        size_t operator()(const PipelineKey& k) const noexcept {
            return (static_cast<size_t>(k.shaderId) * 0x9E3779B97F4A7C15ULL) ^ k.flags;
        }
    };
    std::unordered_map<std::string, uint32_t> shaderIdMap_;
    uint32_t nextShaderId_ = 1;
    std::unordered_map<PipelineKey, RHIPipeline, PipelineKeyHash> scenePipelineCache_;

    // Render pass for blit / overlay (renders to swapchain)
    RHIRenderPass   swapchainPass_  = {};

    // Per-frame scene pass state (set by BeginScenePass, used by granular methods)
    Scene*   currentScene_      = nullptr;
    float    sceneView_[16]     = {};
    float    sceneProj_[16]     = {};
    float    sceneCameraPos_[4] = {};
    float    sceneInvVP_[16]    = {};
    bool     scenePassActive_   = false;

    // Persistent staging buffer + per-canvas overlay textures.
    // Each Canvas2D pointer maps to its own GPU texture so that partial
    // (dirty-rect) uploads don't smear pixels from one canvas into
    // another.  modVersion gates uploads to the frames where the canvas
    // actually changed; lastDirtyRect is unioned with the current dirty
    // rect to also re-upload pixels cleared since the prior frame.
    std::vector<uint8_t> overlayStagingRGBA_;
    struct OverlayCanvasState {
        RHITexture texture     = {};
        uint32_t   texW        = 0;
        uint32_t   texH        = 0;
        uint64_t   lastModVer  = ~uint64_t{0};
        bool       hasUploaded = false;
        int        lastMinX = 0, lastMinY = 0, lastMaxX = -1, lastMaxY = -1;
    };
    std::unordered_map<koilo::Canvas2D*, OverlayCanvasState> overlayStates_;
    /// Set true by StageCanvasOverlays() so that a subsequent
    /// CompositeCanvasOverlays() in the same frame skips the upload step
    /// and only issues fullscreen-quad draws.  Reset in PrepareFrame().
    bool overlayStagedThisFrame_ = false;

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
