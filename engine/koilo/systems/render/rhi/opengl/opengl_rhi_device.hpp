// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file opengl_rhi_device.hpp
 * @brief OpenGL 3.3+ implementation of IRHIDevice.
 *
 * Maps RHI opaque handles to OpenGL objects via internal slot arrays.
 * Receives an OpenGLBackend pointer from the display layer to access
 * the GL context and window.  Unlike Vulkan, GL calls execute
 * immediately - command recording methods emit GL calls directly.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */
#pragma once
#include "../rhi_device.hpp"

#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else
    #include <glad/glad.h>
#endif

#include <vector>
#include <string>
#include <cstdint>
#include <array>

namespace koilo { class OpenGLBackend; }

namespace koilo::rhi {

/// OpenGL 3.3+ implementation of IRHIDevice.
///
/// Slot arrays map uint32_t handle IDs to native GL objects.  Slot 0
/// is reserved (null sentinel).  Freed slots are recycled via a free list.
class OpenGLRHIDevice final : public IRHIDevice {
public:
    explicit OpenGLRHIDevice(OpenGLBackend* display);
    ~OpenGLRHIDevice() override;

    // -- Lifecycle ---------------------------------------------------
    bool Initialize() override;
    void Shutdown()   override;
    const char* GetName() const override { return "OpenGL RHI"; }

    // -- Capabilities ------------------------------------------------
    bool      SupportsFeature(RHIFeature feature) const override;
    RHILimits GetLimits() const override;

    // -- Resource creation / destruction -----------------------------
    RHIBuffer     CreateBuffer(const RHIBufferDesc& desc) override;
    void          DestroyBuffer(RHIBuffer handle) override;

    RHITexture    CreateTexture(const RHITextureDesc& desc) override;
    void          DestroyTexture(RHITexture handle) override;

    RHIShader     CreateShader(RHIShaderStage stage,
                                const void* code, size_t codeSize) override;
    void          DestroyShader(RHIShader handle) override;

    RHIPipeline   CreatePipeline(const RHIPipelineDesc& desc) override;
    void          DestroyPipeline(RHIPipeline handle) override;

    RHIRenderPass CreateRenderPass(const RHIRenderPassDesc& desc) override;
    void          DestroyRenderPass(RHIRenderPass handle) override;

    RHIFramebuffer CreateFramebuffer(RHIRenderPass pass,
                                      const RHITexture* colorAttachments,
                                      uint32_t colorCount,
                                      RHITexture depthAttachment,
                                      uint32_t width, uint32_t height) override;
    void           DestroyFramebuffer(RHIFramebuffer handle) override;

    // -- Data transfer -----------------------------------------------
    void  UpdateBuffer(RHIBuffer handle, const void* data,
                       size_t size, size_t offset) override;
    void  UpdateTexture(RHITexture handle, const void* data,
                        size_t dataSize,
                        uint32_t width, uint32_t height) override;
    void* MapBuffer(RHIBuffer handle) override;
    void  UnmapBuffer(RHIBuffer handle) override;

    // -- Command recording -------------------------------------------
    void BeginFrame() override;
    void BeginRenderPass(RHIRenderPass pass, RHIFramebuffer framebuffer,
                          const RHIClearValue& clear) override;
    void EndRenderPass() override;
    void BindPipeline(RHIPipeline pipeline) override;
    void BindVertexBuffer(RHIBuffer buffer,
                           uint32_t binding, uint64_t offset) override;
    void BindIndexBuffer(RHIBuffer buffer,
                          bool is32Bit, uint64_t offset) override;
    void BindUniformBuffer(RHIBuffer buffer, uint32_t set,
                            uint32_t binding,
                            size_t offset, size_t range) override;
    void BindTexture(RHITexture texture,
                      uint32_t set, uint32_t binding) override;
    void SetViewport(const RHIViewport& vp) override;
    void SetScissor(const RHIScissor& sc) override;
    void PushConstants(RHIShaderStage stages, const void* data,
                        uint32_t size, uint32_t offset) override;
    void Draw(uint32_t vertexCount, uint32_t instanceCount,
               uint32_t firstVertex, uint32_t firstInstance) override;
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                      uint32_t firstIndex, int32_t vertexOffset,
                      uint32_t firstInstance) override;
    void EndFrame() override;
    void Present() override;

    // -- Swapchain ---------------------------------------------------
    void GetSwapchainSize(uint32_t& outWidth,
                           uint32_t& outHeight) const override;
    void OnResize(uint32_t width, uint32_t height) override;

private:
    // -- Slot array for handle - GL object mapping -------------------
    template<typename T>
    struct SlotArray {
        std::vector<T>        slots;
        std::vector<uint32_t> freeList;

        uint32_t Alloc(const T& item) {
            uint32_t id;
            if (!freeList.empty()) {
                id = freeList.back();
                freeList.pop_back();
                slots[id] = item;
            } else {
                id = static_cast<uint32_t>(slots.size());
                slots.push_back(item);
            }
            return id;
        }

        void Free(uint32_t id) {
            if (id == 0 || id >= slots.size()) return;
            slots[id] = T{};
            freeList.push_back(id);
        }

        T& Get(uint32_t id) { return slots[id]; }
        const T& Get(uint32_t id) const { return slots[id]; }
        bool Valid(uint32_t id) const { return id > 0 && id < slots.size(); }
    };

    // -- Native slot entries -----------------------------------------

    struct BufferSlot {
        GLuint buffer       = 0;
        GLenum target       = GL_ARRAY_BUFFER;
        GLsizeiptr size     = 0;
        bool   hostVisible  = false;
        void*  mapped       = nullptr;
        std::vector<uint8_t> shadow; // CPU shadow for uniform bridging
    };

    struct TextureSlot {
        GLuint   texture        = 0;
        GLuint   sampler        = 0;
        uint32_t width          = 0;
        uint32_t height         = 0;
        GLenum   internalFormat = 0;
        GLenum   pixelFormat    = 0;
        GLenum   pixelType      = GL_UNSIGNED_BYTE;
    };

    struct ShaderSlot {
        GLuint         shader = 0;
        RHIShaderStage stage  = RHIShaderStage::Vertex;
    };

    struct PipelineSlot {
        GLuint              program     = 0;
        RHITopology         topology    = RHITopology::TriangleList;
        RHIRasterizerState  rasterizer  = {};
        RHIDepthStencilState depthStencil = {};
        RHIBlendState       blend       = {};
        RHIVertexAttr       attrs[RHIPipelineDesc::kMaxVertexAttrs] = {};
        uint32_t            attrCount   = 0;
        uint32_t            vertexStride = 0;
        bool                hasPushConstantBlock = false;

        // Material param names (declaration order) for name-based bridging.
        struct MatParam {
            std::string name;
            uint8_t     type = 0; // ksl::ParamType
        };
        std::vector<MatParam> materialParams;
    };

    struct RenderPassSlot {
        RHIRenderPassDesc desc = {};
    };

    struct FramebufferSlot {
        GLuint   fbo    = 0;
        uint32_t width  = 0;
        uint32_t height = 0;
    };

    // -- GL helpers --------------------------------------------------

    static GLenum ToGLInternalFormat(RHIFormat fmt);
    static GLenum ToGLPixelFormat(RHIFormat fmt);
    static GLenum ToGLPixelType(RHIFormat fmt);
    static GLenum ToGLTopology(RHITopology t);
    static GLenum ToGLCompareFunc(RHICompareOp op);
    static GLenum ToGLBlendFactor(RHIBlendFactor f);
    static GLenum ToGLBlendOp(RHIBlendOp op);
    static GLint  VertexAttrComponentCount(RHIFormat fmt);
    static GLenum VertexAttrGLType(RHIFormat fmt);

    void ApplyRasterizerState(const RHIRasterizerState& rs);
    void ApplyDepthStencilState(const RHIDepthStencilState& ds);
    void ApplyBlendState(const RHIBlendState& bs);
    void SetupVertexAttributes();

    /// Bridge UBO data to individual glUniform* calls for GLSL shaders
    /// that use old-style `uniform mat4 u_model;` instead of UBO blocks.
    void BridgeTransformUniforms(GLuint program, const uint8_t* data, size_t len);
    void BridgeSceneUniforms(GLuint program, const uint8_t* data, size_t len);
    void BridgeLightUniforms(GLuint program, const uint8_t* data, size_t len);
    void BridgeAudioUniforms(GLuint program, const uint8_t* data, size_t len);
    void BridgeMaterialUniforms(GLuint program, const uint8_t* data, size_t len);

    // -- State -------------------------------------------------------

    OpenGLBackend* display_ = nullptr;
    bool initialized_ = false;

    GLuint vao_ = 0;   // persistent VAO reconfigured per draw
    GLuint pushConstantUBO_ = 0;
    static constexpr uint32_t kPushConstantBinding = 7;
    static constexpr uint32_t kPushConstantMaxSize = 128;

    // Slot arrays
    SlotArray<BufferSlot>      buffers_;
    SlotArray<TextureSlot>     textures_;
    SlotArray<ShaderSlot>      shaders_;
    SlotArray<PipelineSlot>    pipelines_;
    SlotArray<RenderPassSlot>  renderPasses_;
    SlotArray<FramebufferSlot> framebuffers_;

    RHIFeature supportedFeatures_ = static_cast<RHIFeature>(0);
    RHILimits  limits_{};

    // Current frame state
    uint32_t boundPipeline_    = 0;
    uint32_t boundVertexBuf_   = 0;
    uint64_t boundVBOffset_    = 0;
    uint32_t boundIndexBuf_    = 0;
    bool     indexIs32Bit_     = true;
    uint64_t boundIBOffset_    = 0;
    bool     vertexStateDirty_ = true;

    // Tracked UBO bindings for re-applying bridge uniforms on program switch.
    // Key: (set << 16) | binding. Value: buffer handle id.
    static constexpr int kMaxTrackedBindings = 8;
    struct TrackedBinding {
        uint32_t setBinding = 0;  // (set << 16) | binding
        uint32_t bufferId   = 0;
        size_t   offset     = 0;
        size_t   range      = 0;
        bool     active     = false;
    };
    TrackedBinding trackedBindings_[kMaxTrackedBindings];
};

} // namespace koilo::rhi
