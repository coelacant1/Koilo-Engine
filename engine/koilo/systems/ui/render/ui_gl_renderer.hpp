// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_gl_renderer.hpp
 * @brief OpenGL batched UI renderer.
 *
 * Consumes a UIDrawList and renders it using batched quads with
 * a single shader program.  Minimizes draw calls by batching
 * same-texture quads.  Supports solid rects, textured rects
 * (glyph atlas), borders, and scissor clipping.
 *
 * Requires an active OpenGL 3.3+ context.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/ui/render/draw_list.hpp>
#include <koilo/systems/ui/render/ui_vertex.hpp>
#include <koilo/systems/font/font.hpp>
#include "../../../registry/reflect_macros.hpp"

#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else
    #include <glad/glad.h>
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace koilo {
namespace ui {

// --- OpenGL UI Renderer ---------------------------------------------

/** @class UIGLRenderer @brief OpenGL batched UI renderer. */
class UIGLRenderer {
public:
    static constexpr size_t MAX_VERTICES = 65536; ///< maximum vertices per batch
    static constexpr size_t MAX_QUADS = MAX_VERTICES / 6; ///< maximum quads per batch

    UIGLRenderer() = default;
    ~UIGLRenderer() { Shutdown(); }

    // Non-copyable
    UIGLRenderer(const UIGLRenderer&) = delete;
    UIGLRenderer& operator=(const UIGLRenderer&) = delete;

    /// Initialize GL resources. Must be called with active GL context.
    bool Initialize();

    /// Release GL resources.
    void Shutdown();

    /// Upload a font atlas to GPU. Returns GL texture handle.
    GLuint UploadFontAtlas(font::GlyphAtlas& atlas);

    /// Render a draw list to the current framebuffer.
    void Render(const UIDrawList& drawList, int viewportW, int viewportH);

    /** @brief Check whether the renderer is initialized. */
    bool IsInitialized() const { return initialized_; }
    /** @brief Return the GL texture handle for the font atlas. */
    GLuint FontAtlasTexture() const { return fontAtlasTexture_; }

private:
    bool initialized_ = false; ///< true after Initialize() succeeds
    GLuint program_ = 0;       ///< GL shader program handle
    GLuint vao_ = 0, vbo_ = 0; ///< vertex array and buffer objects
    GLuint whiteTexture_ = 0;  ///< 1×1 white texture for solid fills
    GLuint fontAtlasTexture_ = 0; ///< font atlas GL texture
    int fontAtlasW_ = 0, fontAtlasH_ = 0; ///< cached atlas dimensions

    GLint locViewport_ = -1;   ///< uniform location: viewport size
    GLint locTexture_  = -1;   ///< uniform location: texture sampler
    GLint locUseTexture_ = -1; ///< uniform location: texture enable flag

    UIVertex vertices_[MAX_VERTICES]; ///< CPU-side vertex staging buffer
    size_t vertexCount_ = 0;   ///< number of staged vertices
    GLuint currentTexture_ = 0; ///< texture bound for current batch

    void SetTexture(GLuint tex, bool isTextured, bool& useTexture);
    void PushQuad(float x, float y, float w, float h,
                  float u0, float v0, float u1, float v1,
                  Color4 c);
    void PushRoundedQuad(float x, float y, float w, float h,
                         const float radii[4], float borderWidth, Color4 c);
    void EmitBorder(float x, float y, float w, float h,
                    float bw, Color4 c);
    void PushTriangle(float x0, float y0, float x1, float y1,
                      float x2, float y2, Color4 c);
    void Flush(bool useTexture);
    GLuint CompileUIProgram();
    static GLuint CompileProgram(const char* vertSrc, const char* fragSrc);

    std::vector<std::array<GLint, 4>> scissorStack_; ///< nested scissor state stack

    KL_BEGIN_FIELDS(UIGLRenderer)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(UIGLRenderer)
        KL_METHOD_AUTO(UIGLRenderer, IsInitialized, "Is initialized"),
        KL_METHOD_AUTO(UIGLRenderer, FontAtlasTexture, "Font atlas texture")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(UIGLRenderer)
        KL_CTOR0(UIGLRenderer)
    KL_END_DESCRIBE(UIGLRenderer)

};

} // namespace ui
} // namespace koilo
