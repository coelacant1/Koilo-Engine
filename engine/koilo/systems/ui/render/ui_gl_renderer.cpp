// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file ui_gl_renderer.cpp
 * @brief OpenGL batched UI renderer implementation.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#include <koilo/systems/ui/render/ui_gl_renderer.hpp>

#include <algorithm>
#include <iostream>

namespace koilo {
namespace ui {

// --- Public methods -------------------------------------------------

bool UIGLRenderer::Initialize() {
    if (initialized_) return true;

    program_ = CompileUIProgram();
    if (!program_) return false;

    locViewport_ = glGetUniformLocation(program_, "u_viewport");
    locTexture_  = glGetUniformLocation(program_, "u_texture");
    locUseTexture_ = glGetUniformLocation(program_, "u_useTexture");

    // Create 1x1 white texture for solid fills
    glGenTextures(1, &whiteTexture_);
    glBindTexture(GL_TEXTURE_2D, whiteTexture_);
    uint8_t white[] = { 255, 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Create VAO/VBO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 MAX_VERTICES * sizeof(UIVertex),
                 nullptr, GL_STREAM_DRAW);

    // Position (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          sizeof(UIVertex),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // UV (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(UIVertex),
                          reinterpret_cast<void*>(8));
    glEnableVertexAttribArray(1);

    // Color (location 2, normalized ubyte4)
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                          sizeof(UIVertex),
                          reinterpret_cast<void*>(16));
    glEnableVertexAttribArray(2);

    // SDF data (location 3, vec4: halfW, halfH, borderWidth, 0)
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE,
                          sizeof(UIVertex),
                          reinterpret_cast<void*>(20));
    glEnableVertexAttribArray(3);

    // Per-corner radii (location 4, vec4: TL, TR, BR, BL)
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE,
                          sizeof(UIVertex),
                          reinterpret_cast<void*>(36));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    initialized_ = true;
    return true;
}

void UIGLRenderer::Shutdown() {
    if (!initialized_) return;
    if (program_)      { glDeleteProgram(program_); program_ = 0; }
    if (vao_)          { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    if (vbo_)          { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (whiteTexture_) { glDeleteTextures(1, &whiteTexture_); whiteTexture_ = 0; }
    initialized_ = false;
}

GLuint UIGLRenderer::UploadFontAtlas(font::GlyphAtlas& atlas) {
    if (!initialized_) return 0;

    if (fontAtlasTexture_ == 0) {
        glGenTextures(1, &fontAtlasTexture_);
        glBindTexture(GL_TEXTURE_2D, fontAtlasTexture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // Swizzle: alpha-only texture -> use R channel as alpha
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
                     atlas.Width(), atlas.Height(), 0,
                     GL_RED, GL_UNSIGNED_BYTE, atlas.Pixels());
        fontAtlasW_ = atlas.Width();
        fontAtlasH_ = atlas.Height();
    } else if (atlas.IsDirty()) {
        glBindTexture(GL_TEXTURE_2D, fontAtlasTexture_);
        if (atlas.Width() != fontAtlasW_ || atlas.Height() != fontAtlasH_) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
                         atlas.Width(), atlas.Height(), 0,
                         GL_RED, GL_UNSIGNED_BYTE, atlas.Pixels());
            fontAtlasW_ = atlas.Width();
            fontAtlasH_ = atlas.Height();
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                            atlas.Width(), atlas.Height(),
                            GL_RED, GL_UNSIGNED_BYTE, atlas.Pixels());
        }
    }

    atlas.ClearDirty();
    return fontAtlasTexture_;
}

void UIGLRenderer::Render(const UIDrawList& drawList, int viewportW, int viewportH) {
    if (!initialized_ || drawList.Size() == 0) return;

    scissorStack_.clear();

    // Save GL state
    GLboolean prevBlend, prevDepthTest, prevScissorTest;
    GLint prevProgram, prevVao, prevTex;
    GLint prevViewport[4];
    glGetBooleanv(GL_BLEND, &prevBlend);
    glGetBooleanv(GL_DEPTH_TEST, &prevDepthTest);
    glGetBooleanv(GL_SCISSOR_TEST, &prevScissorTest);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Set UI render state
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, viewportW, viewportH);

    glUseProgram(program_);
    glUniform2f(locViewport_,
                static_cast<float>(viewportW),
                static_cast<float>(viewportH));
    glUniform1i(locTexture_, 0);
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    vertexCount_ = 0;
    currentTexture_ = whiteTexture_;
    bool useTexture = false;

    for (size_t i = 0; i < drawList.Size(); ++i) {
        const DrawCmd& cmd = drawList[i];

        switch (cmd.type) {
        case DrawCmdType::SolidRect:
            SetTexture(whiteTexture_, false, useTexture);
            PushQuad(cmd.x, cmd.y, cmd.w, cmd.h,
                     0.0f, 0.0f, 1.0f, 1.0f, cmd.color);
            break;

        case DrawCmdType::BorderRect:
            SetTexture(whiteTexture_, false, useTexture);
            EmitBorder(cmd.x, cmd.y, cmd.w, cmd.h,
                       cmd.borderWidth, cmd.color);
            break;

        case DrawCmdType::TexturedRect:
            SetTexture(cmd.textureHandle ? cmd.textureHandle : whiteTexture_,
                       cmd.textureHandle != 0, useTexture);
            PushQuad(cmd.x, cmd.y, cmd.w, cmd.h,
                     cmd.u0, cmd.v0, cmd.u1, cmd.v1, cmd.color);
            break;

        case DrawCmdType::RoundedRect:
            SetTexture(whiteTexture_, false, useTexture);
            PushRoundedQuad(cmd.x, cmd.y, cmd.w, cmd.h,
                            cmd.cornerRadius, 0.0f, cmd.color);
            break;

        case DrawCmdType::RoundedBorderRect:
            SetTexture(whiteTexture_, false, useTexture);
            PushRoundedQuad(cmd.x, cmd.y, cmd.w, cmd.h,
                            cmd.cornerRadius, cmd.borderWidth, cmd.color);
            break;

        case DrawCmdType::PushScissor: {
            Flush(useTexture);
            int sx = cmd.scissorX;
            int sy = viewportH - cmd.scissorY - cmd.scissorH;
            int sw = cmd.scissorW;
            int sh = cmd.scissorH;
            // Intersect with parent scissor so children can't escape
            if (!scissorStack_.empty()) {
                auto& cur = scissorStack_.back();
                int x1 = std::max(sx, cur[0]);
                int y1 = std::max(sy, cur[1]);
                int x2 = std::min(sx + sw, cur[0] + cur[2]);
                int y2 = std::min(sy + sh, cur[1] + cur[3]);
                sx = x1; sy = y1;
                sw = std::max(0, x2 - x1);
                sh = std::max(0, y2 - y1);
            }
            scissorStack_.push_back({sx, sy, sw, sh});
            glEnable(GL_SCISSOR_TEST);
            glScissor(sx, sy, sw, sh);
            break;
        }

        case DrawCmdType::PopScissor:
            Flush(useTexture);
            if (!scissorStack_.empty()) scissorStack_.pop_back();
            if (!scissorStack_.empty()) {
                auto& prev = scissorStack_.back();
                glScissor(prev[0], prev[1], prev[2], prev[3]);
            } else {
                glDisable(GL_SCISSOR_TEST);
            }
            break;
        }
    }

    Flush(useTexture);

    // Restore GL state
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glBindVertexArray(prevVao);
    glUseProgram(prevProgram);
    glBindTexture(GL_TEXTURE_2D, prevTex);
    if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (!prevBlend) glDisable(GL_BLEND);
    if (prevScissorTest) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
}

// --- Private batching helpers ---------------------------------------

void UIGLRenderer::SetTexture(GLuint tex, bool isTextured, bool& useTexture) {
    if (tex != currentTexture_ || isTextured != useTexture) {
        Flush(useTexture);
        currentTexture_ = tex;
        useTexture = isTextured;
    }
}

void UIGLRenderer::PushQuad(float x, float y, float w, float h,
                             float u0, float v0, float u1, float v1,
                             Color4 c) {
    if (vertexCount_ + 6 > MAX_VERTICES) Flush(false);

    UIVertex* v = &vertices_[vertexCount_];

    // Triangle 1
    v[0] = { x,     y,     u0, v0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };
    v[1] = { x + w, y,     u1, v0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };
    v[2] = { x + w, y + h, u1, v1, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };

    // Triangle 2
    v[3] = { x,     y,     u0, v0, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };
    v[4] = { x + w, y + h, u1, v1, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };
    v[5] = { x,     y + h, u0, v1, c.r, c.g, c.b, c.a, {0,0,0,0}, {0,0,0,0} };

    vertexCount_ += 6;
}

void UIGLRenderer::PushRoundedQuad(float x, float y, float w, float h,
                                    const float radii[4], float borderWidth, Color4 c) {
    if (vertexCount_ + 6 > MAX_VERTICES) Flush(false);

    UIVertex* v = &vertices_[vertexCount_];
    float halfW = w * 0.5f;
    float halfH = h * 0.5f;
    float sdf[4] = { halfW, halfH, borderWidth, 0.0f };
    float rv[4] = { radii[0], radii[1], radii[2], radii[3] };

    // UV (0,0)->(1,1) maps to local normalized position within the rect
    v[0] = { x,     y,     0.0f, 0.0f, c.r, c.g, c.b, c.a, {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]} };
    v[1] = { x + w, y,     1.0f, 0.0f, c.r, c.g, c.b, c.a, {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]} };
    v[2] = { x + w, y + h, 1.0f, 1.0f, c.r, c.g, c.b, c.a, {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]} };

    v[3] = { x,     y,     0.0f, 0.0f, c.r, c.g, c.b, c.a, {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]} };
    v[4] = { x + w, y + h, 1.0f, 1.0f, c.r, c.g, c.b, c.a, {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]} };
    v[5] = { x,     y + h, 0.0f, 1.0f, c.r, c.g, c.b, c.a, {sdf[0],sdf[1],sdf[2],sdf[3]}, {rv[0],rv[1],rv[2],rv[3]} };

    vertexCount_ += 6;
}

void UIGLRenderer::EmitBorder(float x, float y, float w, float h,
                               float bw, Color4 c) {
    // Top
    PushQuad(x, y, w, bw, 0, 0, 1, 1, c);
    // Bottom
    PushQuad(x, y + h - bw, w, bw, 0, 0, 1, 1, c);
    // Left
    PushQuad(x, y + bw, bw, h - bw * 2.0f, 0, 0, 1, 1, c);
    // Right
    PushQuad(x + w - bw, y + bw, bw, h - bw * 2.0f, 0, 0, 1, 1, c);
}

void UIGLRenderer::Flush(bool useTexture) {
    if (vertexCount_ == 0) return;

    glBindTexture(GL_TEXTURE_2D, currentTexture_);
    glUniform1i(locUseTexture_, useTexture ? 1 : 0);

    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    vertexCount_ * sizeof(UIVertex), vertices_);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount_));

    vertexCount_ = 0;
}

// --- Shader compilation ---------------------------------------------

GLuint UIGLRenderer::CompileUIProgram() {
    const char* vertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;
layout(location = 3) in vec4 a_sdf;   // (halfW, halfH, borderWidth, 0)
layout(location = 4) in vec4 a_radii; // (TL, TR, BR, BL)

uniform vec2 u_viewport;

out vec2 v_uv;
out vec4 v_color;
out vec4 v_sdf;
out vec4 v_radii;

void main() {
    // Convert pixel coordinates to NDC (-1..1)
    vec2 ndc = (a_position / u_viewport) * 2.0 - 1.0;
    ndc.y = -ndc.y;  // flip Y (screen Y-down -> GL Y-up)
    gl_Position = vec4(ndc, 0.0, 1.0);
    v_uv = a_uv;
    v_color = a_color;
    v_sdf = a_sdf;
    v_radii = a_radii;
}
)GLSL";

    const char* fragSrc = R"GLSL(
#version 330 core
in vec2 v_uv;
in vec4 v_color;
in vec4 v_sdf;   // (halfW, halfH, borderWidth, 0)
in vec4 v_radii; // (TL, TR, BR, BL)

uniform sampler2D u_texture;
uniform int u_useTexture;

out vec4 FragColor;

// Per-corner SDF: selects correct radius based on quadrant
// p: centered coords, b: half-size, r: (TL, TR, BR, BL)
// In our coord system: +x=right, +y=down (screen space after UV mapping)
float roundedRectSDF(vec2 p, vec2 b, vec4 r) {
    // Select radius based on quadrant
    // p.x > 0 -> right side (TR or BR), p.x < 0 -> left side (TL or BL)
    // p.y > 0 -> bottom (BR or BL),    p.y < 0 -> top (TL or TR)
    float cr = (p.x > 0.0)
        ? ((p.y > 0.0) ? r.z : r.y)   // right: bottom=BR, top=TR
        : ((p.y > 0.0) ? r.w : r.x);  // left:  bottom=BL, top=TL
    vec2 q = abs(p) - b + vec2(cr);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - cr;
}

void main() {
    float halfW = v_sdf.x;
    float halfH = v_sdf.y;
    float borderW = v_sdf.z;
    bool hasRadius = (v_radii.x > 0.0 || v_radii.y > 0.0 || v_radii.z > 0.0 || v_radii.w > 0.0);

    if (halfW > 0.0 && hasRadius) {
        // SDF rounded rect mode
        vec2 halfSize = vec2(halfW, halfH);
        // Map UV (0..1) to local centered coordinates (-halfW..halfW, -halfH..halfH)
        vec2 p = v_uv * halfSize * 2.0 - halfSize;

        float outerD = roundedRectSDF(p, halfSize, v_radii);
        float outerAlpha = 1.0 - smoothstep(-0.75, 0.75, outerD);

        if (borderW > 0.0) {
            // Border mode: hollow out the inside
            vec4 innerR = max(v_radii - vec4(borderW), vec4(0.0));
            vec2 innerHalf = halfSize - vec2(borderW);
            float innerD = roundedRectSDF(p, innerHalf, innerR);
            float innerAlpha = 1.0 - smoothstep(-0.75, 0.75, innerD);
            FragColor = vec4(v_color.rgb, v_color.a * (outerAlpha - innerAlpha));
        } else {
            // Filled rounded rect
            FragColor = vec4(v_color.rgb, v_color.a * outerAlpha);
        }
    } else if (u_useTexture != 0) {
        float alpha = texture(u_texture, v_uv).a;
        FragColor = vec4(v_color.rgb, v_color.a * alpha);
    } else {
        FragColor = v_color;
    }
}
)GLSL";

    return CompileProgram(vertSrc, fragSrc);
}

GLuint UIGLRenderer::CompileProgram(const char* vertSrc, const char* fragSrc) {
    auto compileShader = [](GLenum type, const char* src) -> GLuint {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        GLint ok;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::cerr << "[UIGLRenderer] Shader error:\n"
                      << log << std::endl;
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    };

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc);
    if (!vs) return 0;
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!fs) { glDeleteShader(vs); return 0; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "[UIGLRenderer] Link error:\n"
                  << log << std::endl;
        glDeleteProgram(prog);
        prog = 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

} // namespace ui
} // namespace koilo
