// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file opengl_render_backend.hpp
 * @brief OpenGL GPU-accelerated render backend.
 *
 * Renders scenes using OpenGL 3.3+ with FBO render targets.
 * Supports mesh VBO/VAO upload, material-to-GLSL shader mapping,
 * and ReadPixels for CPU post-processing (Canvas2D, modules).
 *
 * Requires an active OpenGL context (from OpenGLBackend display).
 *
 * @date 23/02/2026
 * @author Coela
 */

#pragma once

#include <koilo/systems/render/irenderbackend.hpp>
#include <koilo/systems/render/gl/software_render_backend.hpp>
#include <koilo/ksl/ksl_registry.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

namespace koilo {

class Mesh;
class Light;
class Texture;
class KSLMaterial;
template<typename, typename> class MaterialT;

/**
 * @class OpenGLRenderBackend
 * @brief GPU-accelerated rendering backend using OpenGL 3.3+.
 *
 * Renders 3D scenes to an FBO, then provides ReadPixels for CPU compositing.
 * Meshes with unsupported shaders fall back to software rasterization.
 */
class OpenGLRenderBackend : public IRenderBackend {
public:
    OpenGLRenderBackend();
    ~OpenGLRenderBackend() override;

    bool Initialize() override;
    void Shutdown() override;
    bool IsInitialized() const override;
    void Render(Scene* scene, CameraBase* camera) override;
    /** Render without kicking the PBO readback (for direct-blit path). */
    void RenderDirect(Scene* scene, CameraBase* camera);
    void ReadPixels(Color888* buffer, int width, int height) override;
    const char* GetName() const override;

    /** @brief Blit the render FBO directly to the default framebuffer (screen).
     *  Skips ReadPixels entirely - use when no CPU compositing is needed.
     *  Call after Render(), then SDL_GL_SwapWindow. */
    void BlitToScreen(int screenW, int screenH);

    /** @brief Composite Canvas2D overlays onto the default framebuffer. */
    void CompositeCanvasOverlays(int screenW, int screenH);

    /** Cached KSL uniform location lookup. */
    int GetKSLUniform(unsigned int prog, const std::string& name);

private:
    // FBO resources (use unsigned int to avoid GL header dependency)
    unsigned int fbo_;
    unsigned int colorTex_;
    unsigned int depthRbo_;
    int fbWidth_;
    int fbHeight_;

    // Shader programs
    unsigned int sceneProgram_;

    // Sky quad resources
    unsigned int skyVao_;
    unsigned int skyVbo_;

    // Mesh VBO cache
    struct MeshCacheEntry {
        unsigned int vao;
        unsigned int vbo;
        int vertexCount;
        uint32_t lastVersion = 0;
    };
    std::unordered_map<uintptr_t, MeshCacheEntry> meshCache_;

    // Texture cache for GPU upload
    std::unordered_map<uintptr_t, unsigned int> textureCache_;

    // Cached uniform locations per shader program
    struct UniformLocs {
        int model = -1, view = -1, projection = -1;
        int time = -1, cameraPos = -1, lightCount = -1;
    };
    std::unordered_map<unsigned int, UniformLocs> uniformCache_;
    const UniformLocs& GetUniforms(unsigned int prog);

    // Per-program KSL param uniform location cache (prog -> name -> location)
    std::unordered_map<unsigned int, std::unordered_map<std::string, int>> kslUniformCache_;

    // KSL shader registry (loads .glsl from build/shaders/)
    ksl::KSLRegistry kslRegistry_;

    // Pink error fallback program
    unsigned int pinkProgram_;

    // PBO double-buffer for async pixel readback
    unsigned int pbo_[2];
    int pboIndex_;
    bool pboReady_;

    // CPU readback buffer
    std::vector<uint8_t> readbackBuf_;

    bool initialized_;
    SoftwareRenderBackend fallback_;

    // Internal methods
    bool InitShaders();
    bool InitFBO(int width, int height);
    void ResizeFBO(int width, int height);
    unsigned int CompileProgram(const char* vertSrc, const char* fragSrc);
    MeshCacheEntry& UploadMesh(Mesh* mesh);
    void RenderSky(CameraBase* camera, int vpW, int vpH);
    void CleanupMeshCache();
    void InitShaderRegistry();
    void SetLightUniforms(unsigned int prog, const std::vector<Light>& lights);
    unsigned int UploadTexture(Texture* tex);
    void BindKSLTextures(unsigned int prog, const KSLMaterial& kmat);
    void CleanupTextureCache();

    // Canvas overlay resources
    unsigned int overlayProgram_;
    unsigned int overlayVao_;
    unsigned int overlayVbo_;
    unsigned int overlayTex_;
    int overlayTexW_, overlayTexH_;
    std::vector<uint8_t> overlayRgba_; // persistent RGBA buffer

    // Debug line rendering resources
    unsigned int lineProgram_;
    unsigned int lineVao_;
    unsigned int lineVbo_;
    void RenderDebugLines(const float* viewMat, const float* projMat);

    // Batch rendering resources (merged same-material draw calls)
    unsigned int batchVao_;
    unsigned int batchVbo_;
    long batchVboSize_;

    KL_BEGIN_FIELDS(OpenGLRenderBackend)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(OpenGLRenderBackend)
        KL_METHOD_AUTO(OpenGLRenderBackend, Initialize, "Initialize"),
        KL_METHOD_AUTO(OpenGLRenderBackend, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(OpenGLRenderBackend, IsInitialized, "Is initialized"),
        KL_METHOD_AUTO(OpenGLRenderBackend, Render, "Render"),
        KL_METHOD_AUTO(OpenGLRenderBackend, ReadPixels, "Read pixels"),
        KL_METHOD_AUTO(OpenGLRenderBackend, GetName, "Get name")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(OpenGLRenderBackend)
        KL_CTOR0(OpenGLRenderBackend)
    KL_END_DESCRIBE(OpenGLRenderBackend)
};

} // namespace koilo
