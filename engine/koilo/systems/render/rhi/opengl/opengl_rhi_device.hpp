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
#include <deque>
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
    bool  SupportsTextureSubUpdate() const override { return true; }
    void  UpdateTextureRegion(RHITexture handle, const void* data,
                              size_t dataSize,
                              uint32_t x, uint32_t y,
                              uint32_t w, uint32_t h) override;
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

    // -- Timestamp queries -------------------------------------------
    void   ResetTimestamps(uint32_t maxQueries) override;
    void   WriteTimestamp(uint32_t index) override;
    bool   ReadTimestamps(uint64_t* out, uint32_t count) override;
    double GetTimestampPeriod() const override;
    void   WaitIdle() override;

private:
    // -- Slot array for handle - GL object mapping -------------------
    // Storage is std::deque<T> so references to existing slots remain
    // valid after Alloc() growth. This lets the device cache resolved
    // slot pointers (boundPipelinePtr_, boundVertexBufPtr_, etc.) at
    // Bind* time and reuse them on every Draw without re-indexing. (C2)
    template<typename T>
    struct SlotArray {
        std::deque<T>         slots;
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
        T* GetPtr(uint32_t id) {
            return (id > 0 && id < slots.size()) ? &slots[id] : nullptr;
        }
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

        // -- B3: cached uniform locations -----------------------------
        // Populated lazily on the first Bridge*Uniforms call for this
        // pipeline.  -1 means "not present in this program"; -2 means
        // "not yet queried".  Avoids per-frame glGetUniformLocation
        // string lookups (expensive on Pi/embedded GL drivers).
        struct StdUniformLocs {
            GLint model            = -2;
            GLint view             = -2;
            GLint projection       = -2;
            GLint cameraPos        = -2;
            GLint inverseViewProj  = -2;
            GLint lightCount       = -2;
            // Per-light arrays: [i].position/.color/.intensity/.falloff/.curve
            GLint lightPos[16]     = {-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2};
            GLint lightCol[16]     = {-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2};
            GLint lightInt[16]     = {-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2};
            GLint lightFalloff[16] = {-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2};
            GLint lightCurve[16]   = {-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2};
        };
        StdUniformLocs stdLocs;

        // Material param names (declaration order) for name-based bridging.
        struct MatParam {
            std::string name;
            uint8_t     type = 0; // ksl::ParamType
            GLint       cachedLoc = -2;  // B3: cached uniform location
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
    void BridgeTransformUniforms(PipelineSlot& pipe, const uint8_t* data, size_t len);
    void BridgeSceneUniforms(PipelineSlot& pipe, const uint8_t* data, size_t len);
    void BridgeLightUniforms(PipelineSlot& pipe, const uint8_t* data, size_t len);
    void BridgeAudioUniforms(PipelineSlot& pipe, const uint8_t* data, size_t len);
    void BridgeMaterialUniforms(PipelineSlot& pipe, const uint8_t* data, size_t len);

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

    // C2: cached resolved slot pointers for the hot draw path.
    // Set by BindPipeline / BindVertexBuffer / BindIndexBuffer; cleared by
    // the matching Destroy*. Storage is std::deque so these stay valid
    // across subsequent Alloc()s.
    PipelineSlot* boundPipelinePtr_  = nullptr;
    BufferSlot*   boundVertexBufPtr_ = nullptr;
    BufferSlot*   boundIndexBufPtr_  = nullptr;

    // Tracked UBO bindings, indexed directly by (set * kMaxBindingsPerSet + binding)
    // for O(1) lookup on every BindUniformBuffer + BindPipeline. (C3)
    static constexpr int kMaxBindingSets        = 2;
    static constexpr int kMaxBindingsPerSet     = 4;
    static constexpr int kMaxTrackedBindings    = kMaxBindingSets * kMaxBindingsPerSet;
    struct TrackedBinding {
        uint32_t bufferId   = 0;
        size_t   offset     = 0;
        size_t   range      = 0;
        bool     active     = false;
    };
    TrackedBinding trackedBindings_[kMaxTrackedBindings];

    // Bitmask of vertex-attribute slots currently enabled in the VAO.
    // Used by SetupVertexAttributes() to disable only those slots that
    // were enabled by the previous pipeline but are not used by the
    // current one - instead of unconditionally disabling all 8 slots
    // every draw.
    uint32_t lastAttrMask_ = 0;

    // -- Timestamp query state ---------------------------------------
    std::vector<GLuint> tsQueries_;        // GL query objects
    std::vector<GLuint> tsQueriesPrev_;    // Previous frame's queries (for readback)
    uint32_t            tsWriteCount_ = 0; // Timestamps written this frame
    uint32_t            tsPrevCount_  = 0; // Timestamps available for readback
    bool                tsSupported_  = false;
};

} // namespace koilo::rhi
