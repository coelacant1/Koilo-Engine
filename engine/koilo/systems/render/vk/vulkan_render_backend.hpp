// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file vulkan_render_backend.hpp
 * @brief Vulkan GPU-accelerated render backend.
 *
 * Renders scenes using Vulkan with off-screen render targets.
 * Supports mesh VBO upload, KSL SPIR-V shader pipelines,
 * descriptor-set-based material binding, and ReadPixels for CPU
 * post-processing.
 *
 * Requires a VulkanBackend display backend for device handles.
 *
 * @date 01/27/2026
 * @author Coela
 */

#pragma once

#include <koilo/systems/render/igpu_render_backend.hpp>
#include <koilo/ksl/ksl_registry.hpp>
#include <koilo/registry/reflect_macros.hpp>

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cstdint>

namespace koilo {

class Mesh;
class Light;
class Texture;
class KSLMaterial;
class VulkanBackend;

/**
 * @class VulkanRenderBackend
 * @brief GPU-accelerated rendering backend using Vulkan.
 *
 * Renders 3D scenes to an off-screen render target, then provides
 * BlitToScreen to composite onto the swapchain image.
 */
class VulkanRenderBackend : public IGPURenderBackend {
public:
    explicit VulkanRenderBackend(VulkanBackend* display);
    ~VulkanRenderBackend() override;

    bool Initialize() override;
    void Shutdown() override;
    bool IsInitialized() const override;
    void Render(Scene* scene, CameraBase* camera) override;
    void RenderDirect(Scene* scene, CameraBase* camera) override;
    void ReadPixels(Color888* buffer, int width, int height) override;
    const char* GetName() const override;

    void BlitToScreen(int screenW, int screenH) override;
    void CompositeCanvasOverlays(int screenW, int screenH) override;
    void PrepareFrame() override;
    void FinishFrame() override;

private:
    VulkanBackend* display_;
    bool initialized_ = false;

    // Device handles (cached from display backend)
    VkDevice         device_         = VK_NULL_HANDLE;
    VkPhysicalDevice physDevice_     = VK_NULL_HANDLE;
    VkQueue          graphicsQueue_  = VK_NULL_HANDLE;
    uint32_t         graphicsFamily_ = 0;

    // Off-screen render target
    VkImage        colorImage_      = VK_NULL_HANDLE;
    VkDeviceMemory colorMemory_     = VK_NULL_HANDLE;
    VkImageView    colorView_       = VK_NULL_HANDLE;
    VkImage        depthImage_      = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory_     = VK_NULL_HANDLE;
    VkImageView    depthView_       = VK_NULL_HANDLE;
    VkRenderPass   offscreenPass_   = VK_NULL_HANDLE;
    VkFramebuffer  offscreenFB_     = VK_NULL_HANDLE;
    VkSampler      colorSampler_    = VK_NULL_HANDLE;
    int fbWidth_ = 0, fbHeight_ = 0;

    // Command pool for render commands
    VkCommandPool   cmdPool_   = VK_NULL_HANDLE;
    VkCommandBuffer cmdBuffer_ = VK_NULL_HANDLE;

    // Descriptor set layouts
    VkDescriptorSetLayout sceneLayout_    = VK_NULL_HANDLE; // Set 0: transform + lights
    VkDescriptorSetLayout materialLayout_ = VK_NULL_HANDLE; // Set 1: material UBO
    VkDescriptorSetLayout textureLayout_  = VK_NULL_HANDLE; // Set 2: sampler(s)

    // Pipeline layout (shared by all KSL pipelines)
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;

    // Descriptor pool
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;

    // Per-frame scene descriptor set (transform + light UBO/SSBO)
    VkDescriptorSet sceneDescSet_ = VK_NULL_HANDLE;

    // Scene uniform buffer (TransformUBO: model/view/proj/cameraPos)
    // Allocated with 2 regions: offset 0 for meshes, offset alignedSize for sky
    struct TransformUBO {
        float model[16];
        float view[16];
        float projection[16];
        float cameraPos[4]; // vec3 + padding
    };
    VkBuffer       transformUBO_    = VK_NULL_HANDLE;
    VkDeviceMemory transformMemory_ = VK_NULL_HANDLE;
    void*          transformMapped_ = nullptr;
    uint32_t       transformAlignedSize_ = 0; // aligned size per region

    // Scene UBO (SceneUBO: lightCount + padding)
    struct SceneUBO {
        int32_t lightCount;
        int32_t _pad1, _pad2, _pad3;
    };
    VkBuffer       sceneUBO_       = VK_NULL_HANDLE;
    VkDeviceMemory sceneUBOMemory_ = VK_NULL_HANDLE;
    void*          sceneUBOMapped_ = nullptr;

    // Light SSBO
    struct GPULight {
        float position[3];
        float intensity;
        float color[3];
        float falloff;
        float curve;
        float _pad0, _pad1, _pad2;
    };
    VkBuffer       lightSSBO_       = VK_NULL_HANDLE;
    VkDeviceMemory lightSSBOMemory_ = VK_NULL_HANDLE;
    void*          lightSSBOMapped_ = nullptr;
    static constexpr int kMaxLights = 16;

    // Audio SSBO (binding 3) - placeholder for audio-reactive shaders
    VkBuffer       audioSSBO_       = VK_NULL_HANDLE;
    VkDeviceMemory audioSSBOMemory_ = VK_NULL_HANDLE;

    // Interleaved vertex format (matches OpenGL: pos3 + normal3 + uv2)
    struct VkVertex {
        float px, py, pz;
        float nx, ny, nz;
        float u, v;
    };

    // Mesh cache
    struct MeshCacheEntry {
        VkBuffer       buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        int vertexCount = 0;
        uint32_t lastVersion = 0;
    };
    std::unordered_map<uintptr_t, MeshCacheEntry> meshCache_;

    // Texture cache
    struct TextureCacheEntry {
        VkImage        image   = VK_NULL_HANDLE;
        VkDeviceMemory memory  = VK_NULL_HANDLE;
        VkImageView    view    = VK_NULL_HANDLE;
        VkSampler      sampler = VK_NULL_HANDLE;
        VkDescriptorSet descSet = VK_NULL_HANDLE;
    };
    std::unordered_map<uintptr_t, TextureCacheEntry> textureCache_;

    // Material UBO cache (per-material)
    struct MaterialCacheEntry {
        VkBuffer        ubo    = VK_NULL_HANDLE;
        VkDeviceMemory  memory = VK_NULL_HANDLE;
        VkDescriptorSet descSet = VK_NULL_HANDLE;
        void*           mapped  = nullptr;
        size_t          size    = 0;
    };
    std::unordered_map<uintptr_t, MaterialCacheEntry> materialCache_;

    // Pipeline cache (per SPIR-V shader name)
    struct PipelineCacheEntry {
        VkPipeline       pipeline = VK_NULL_HANDLE;
        VkShaderModule   fragModule = VK_NULL_HANDLE;
    };
    std::unordered_map<std::string, PipelineCacheEntry> pipelineCache_;

    VkShaderModule vertModule_ = VK_NULL_HANDLE;
    VkPipeline     pinkPipeline_ = VK_NULL_HANDLE; // Error fallback

    // Blit pipeline (fullscreen quad from off-screen to swapchain)
    VkPipeline             blitPipeline_       = VK_NULL_HANDLE;
    VkPipelineLayout       blitPipelineLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout  blitDescLayout_     = VK_NULL_HANDLE;
    static constexpr int   kBlitFrames_        = 2; // matches kMaxFramesInFlight
    VkDescriptorSet        blitDescSets_[2]    = {};
    VkShaderModule         blitVertModule_     = VK_NULL_HANDLE;
    VkShaderModule         blitFragModule_     = VK_NULL_HANDLE;

    // Sky pipeline (no depth test - background)
    VkPipeline     skyPipeline_    = VK_NULL_HANDLE;
    VkShaderModule skyFragModule_  = VK_NULL_HANDLE;

    // Overlay pipeline (Canvas2D compositing)
    VkPipeline             overlayPipeline_       = VK_NULL_HANDLE;
    VkBuffer               overlayBuffer_         = VK_NULL_HANDLE;
    VkDeviceMemory         overlayMemory_         = VK_NULL_HANDLE;
    VkImage                overlayImage_           = VK_NULL_HANDLE;
    VkDeviceMemory         overlayImageMemory_    = VK_NULL_HANDLE;
    VkImageView            overlayImageView_      = VK_NULL_HANDLE;
    VkSampler              overlaySampler_         = VK_NULL_HANDLE;
    VkDescriptorSet        overlayDescSet_        = VK_NULL_HANDLE;
    int overlayTexW_ = 0, overlayTexH_ = 0;
    std::vector<uint8_t>   overlayRgba_;

    // Persistent overlay staging buffer (avoids per-frame alloc/free)
    VkBuffer       overlayStagingBuf_  = VK_NULL_HANDLE;
    VkDeviceMemory overlayStagingMem_  = VK_NULL_HANDLE;
    VkDeviceSize   overlayStagingSize_ = 0;

    // Readback staging buffer
    VkBuffer       readbackBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory readbackMemory_ = VK_NULL_HANDLE;
    void*          readbackMapped_ = nullptr;
    size_t         readbackSize_   = 0;

    // Sky quad vertex buffer
    VkBuffer       skyVBO_      = VK_NULL_HANDLE;
    VkDeviceMemory skyMemory_   = VK_NULL_HANDLE;

    // Fullscreen quad vertex buffer (for blit/overlay)
    VkBuffer       quadVBO_    = VK_NULL_HANDLE;
    VkDeviceMemory quadMemory_ = VK_NULL_HANDLE;

    // Debug line resources
    VkPipeline       linePipeline_   = VK_NULL_HANDLE;
    VkShaderModule   lineVertModule_ = VK_NULL_HANDLE;
    VkShaderModule   lineFragModule_ = VK_NULL_HANDLE;
    VkPipelineLayout linePipeLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout lineDescLayout_ = VK_NULL_HANDLE;
    VkDescriptorSet  lineDescSet_    = VK_NULL_HANDLE;
    VkBuffer         lineVBO_        = VK_NULL_HANDLE;
    VkDeviceMemory   lineMemory_     = VK_NULL_HANDLE;
    VkBuffer         lineUBO_        = VK_NULL_HANDLE;
    VkDeviceMemory   lineUBOMemory_  = VK_NULL_HANDLE;
    void*            lineUBOMapped_  = nullptr;
    size_t           lineVBOSize_    = 0;

    // Batch rendering merged VBO
    VkBuffer       batchVBO_    = VK_NULL_HANDLE;
    VkDeviceMemory batchMemory_ = VK_NULL_HANDLE;
    size_t         batchVBOSize_ = 0;

    // KSL shader registry (for SPIR-V data)
    ksl::KSLRegistry kslRegistry_;

    // Internal initialization
    bool CreateOffscreenTarget(int width, int height);
    void DestroyOffscreenTarget();
    bool CreateDescriptorLayouts();
    bool CreateDescriptorPool();
    bool CreateSceneDescriptors();
    bool CreatePipelineLayout();
    bool CreateShaderModules();
    bool CreateScenePipeline(const std::string& name,
                             const std::vector<uint32_t>& fragSPV,
                             bool depthTest, bool depthWrite,
                             VkPipeline& outPipeline,
                             VkShaderModule& outModule);
    bool CreateBlitPipeline();
    bool CreateOverlayPipeline();
    bool CreateLinePipeline();
    bool CreateSkyResources();
    bool CreateQuadVBO();
    void InitShaderPipelines();
    VkShaderModule LoadSPIRVFile(const char* path);

    // Buffer helpers
    bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags props,
                      VkBuffer& buffer, VkDeviceMemory& memory);
    bool CreateBufferMapped(VkDeviceSize size, VkBufferUsageFlags usage,
                            VkBuffer& buffer, VkDeviceMemory& memory,
                            void** mapped);
    void DestroyBuffer(VkBuffer& buffer, VkDeviceMemory& memory);
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props);

    // Image helpers
    bool CreateImage(uint32_t w, uint32_t h, VkFormat fmt,
                     VkImageUsageFlags usage, VkImage& img,
                     VkDeviceMemory& mem);
    VkImageView CreateImageView(VkImage image, VkFormat format,
                                VkImageAspectFlags aspect);
    void TransitionImageLayout(VkCommandBuffer cmd, VkImage image,
                               VkImageLayout oldLayout, VkImageLayout newLayout,
                               VkImageAspectFlags aspect);

    // One-shot command helpers
    VkCommandBuffer BeginOneShot();
    void EndOneShot(VkCommandBuffer cmd);

    // Mesh/texture management
    MeshCacheEntry& UploadMesh(Mesh* mesh);
    TextureCacheEntry& UploadTexture(Texture* tex);
    MaterialCacheEntry& GetOrCreateMaterial(const KSLMaterial* kmat);
    void UpdateMaterialUBO(MaterialCacheEntry& entry, const KSLMaterial& kmat);
    void CleanupMeshCache();
    void CleanupTextureCache();
    void CleanupMaterialCache();
    void CleanupPipelineCache();

    // Rendering helpers
    void RenderSky(VkCommandBuffer cmd, CameraBase* camera, int vpW, int vpH);
    void RenderDebugLines(const float* viewMat, const float* projMat);
    VkPipeline GetPipelineForMaterial(const KSLMaterial* kmat);

    #ifdef KL_HAVE_VULKAN_BACKEND
    KL_BEGIN_FIELDS(VulkanRenderBackend)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(VulkanRenderBackend)
        KL_METHOD_AUTO(VulkanRenderBackend, Initialize, "Initialize"),
        KL_METHOD_AUTO(VulkanRenderBackend, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(VulkanRenderBackend, IsInitialized, "Is initialized"),
        KL_METHOD_AUTO(VulkanRenderBackend, Render, "Render"),
        KL_METHOD_AUTO(VulkanRenderBackend, RenderDirect, "Render direct"),
        KL_METHOD_AUTO(VulkanRenderBackend, ReadPixels, "Read pixels"),
        KL_METHOD_AUTO(VulkanRenderBackend, GetName, "Get name"),
        KL_METHOD_AUTO(VulkanRenderBackend, BlitToScreen, "Blit to screen"),
        KL_METHOD_AUTO(VulkanRenderBackend, CompositeCanvasOverlays, "Composite canvas overlays"),
        KL_METHOD_AUTO(VulkanRenderBackend, PrepareFrame, "Prepare frame"),
        KL_METHOD_AUTO(VulkanRenderBackend, FinishFrame, "Finish frame")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(VulkanRenderBackend)
        /* No reflected ctors - requires VulkanBackend* (not reflectable). */
    KL_END_DESCRIBE(VulkanRenderBackend)
    #endif

};

} // namespace koilo
