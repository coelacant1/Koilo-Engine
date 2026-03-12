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

// --- UI vertex (16 bytes, packed) -----------------------------------

struct UIVertex {
    float x, y;         // screen-space position
    float u, v;         // texture coordinates / local normalized pos
    uint8_t r, g, b, a; // vertex color
    float sdf[4];       // (halfW, halfH, borderWidth, 0) - 0 when unused
    float radii[4];     // per-corner radius (TL, TR, BR, BL) - 0 when unused

    KL_BEGIN_FIELDS(UIVertex)
        KL_FIELD(UIVertex, y, "Y", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(UIVertex)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(UIVertex)
        /* No reflected ctors. */
    KL_END_DESCRIBE(UIVertex)

};
static_assert(sizeof(UIVertex) == 52, "UIVertex should be 52 bytes");

// --- OpenGL UI Renderer ---------------------------------------------

class UIGLRenderer {
public:
    static constexpr size_t MAX_VERTICES = 65536;
    static constexpr size_t MAX_QUADS = MAX_VERTICES / 6;

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

    bool IsInitialized() const { return initialized_; }
    GLuint FontAtlasTexture() const { return fontAtlasTexture_; }

private:
    bool initialized_ = false;
    GLuint program_ = 0;
    GLuint vao_ = 0, vbo_ = 0;
    GLuint whiteTexture_ = 0;
    GLuint fontAtlasTexture_ = 0;
    int fontAtlasW_ = 0, fontAtlasH_ = 0;

    GLint locViewport_ = -1;
    GLint locTexture_  = -1;
    GLint locUseTexture_ = -1;

    UIVertex vertices_[MAX_VERTICES];
    size_t vertexCount_ = 0;
    GLuint currentTexture_ = 0;

    void SetTexture(GLuint tex, bool isTextured, bool& useTexture);
    void PushQuad(float x, float y, float w, float h,
                  float u0, float v0, float u1, float v1,
                  Color4 c);
    void PushRoundedQuad(float x, float y, float w, float h,
                         const float radii[4], float borderWidth, Color4 c);
    void EmitBorder(float x, float y, float w, float h,
                    float bw, Color4 c);
    void Flush(bool useTexture);
    GLuint CompileUIProgram();
    static GLuint CompileProgram(const char* vertSrc, const char* fragSrc);

    std::vector<std::array<GLint, 4>> scissorStack_;

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
