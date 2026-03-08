// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file debugdraw.hpp
 * @brief Debug visualization for lines, shapes, and text in 3D space.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <vector>
#include <cstdint>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/matrix4x4.hpp>
#include <koilo/core/platform/ustring.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @struct Color
 * @brief RGBA color for debug drawing.
 */
struct Color {
    float r, g, b, a;

    Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    // Common colors
    static const Color White;
    static const Color Black;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Cyan;
    static const Color Magenta;
    static const Color Orange;
    static const Color Purple;
    static const Color Gray;

    KL_BEGIN_FIELDS(Color)
        KL_FIELD(Color, r, "R", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(Color, g, "G", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(Color, b, "B", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(Color, a, "A", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Color)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Color)
        KL_CTOR0(Color),
        KL_CTOR(Color, float, float, float, float)
    KL_END_DESCRIBE(Color)

};

/**
 * @enum DebugDrawMode
 * @brief Drawing mode for debug primitives.
 */
enum class DebugDrawMode : uint8_t {
    Wireframe = 0,  // Draw only edges
    Solid = 1       // Draw filled
};

/**
 * @struct DebugLine
 * @brief A single debug line.
 */
struct DebugLine {
    Vector3D start;
    Vector3D end;
    Color color;
    float duration;    // Duration in seconds (0 = one frame)
    bool depthTest;    // If false, draws on top of everything

    KL_BEGIN_FIELDS(DebugLine)
        KL_FIELD(DebugLine, start, "Start", -2147483648, 2147483647),
        KL_FIELD(DebugLine, end, "End", -2147483648, 2147483647),
        KL_FIELD(DebugLine, color, "Color", 0, 0),
        KL_FIELD(DebugLine, duration, "Duration", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(DebugLine, depthTest, "Depth test", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(DebugLine)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(DebugLine)
        /* No reflected ctors. */
    KL_END_DESCRIBE(DebugLine)

};

/**
 * @struct DebugSphere
 * @brief A debug sphere.
 */
struct DebugSphere {
    Vector3D center;
    float radius;
    Color color;
    float duration;
    bool depthTest;
    DebugDrawMode mode;

    KL_BEGIN_FIELDS(DebugSphere)
        KL_FIELD(DebugSphere, center, "Center", -2147483648, 2147483647),
        KL_FIELD(DebugSphere, radius, "Radius", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(DebugSphere, color, "Color", 0, 0),
        KL_FIELD(DebugSphere, duration, "Duration", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(DebugSphere, depthTest, "Depth test", 0, 1),
        KL_FIELD(DebugSphere, mode, "Mode", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(DebugSphere)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(DebugSphere)
        /* No reflected ctors. */
    KL_END_DESCRIBE(DebugSphere)

};

/**
 * @struct DebugBox
 * @brief A debug box (axis-aligned or oriented).
 */
struct DebugBox {
    Vector3D center;
    Vector3D extents;  // Half-extents
    Matrix4x4 transform;  // For oriented boxes
    Color color;
    float duration;
    bool depthTest;
    DebugDrawMode mode;

    KL_BEGIN_FIELDS(DebugBox)
        KL_FIELD(DebugBox, center, "Center", -2147483648, 2147483647),
        KL_FIELD(DebugBox, extents, "Extents", -2147483648, 2147483647),
        KL_FIELD(DebugBox, transform, "Transform", -2147483648, 2147483647),
        KL_FIELD(DebugBox, color, "Color", 0, 0),
        KL_FIELD(DebugBox, duration, "Duration", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(DebugBox, depthTest, "Depth test", 0, 1),
        KL_FIELD(DebugBox, mode, "Mode", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(DebugBox)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(DebugBox)
        /* No reflected ctors. */
    KL_END_DESCRIBE(DebugBox)

};

/**
 * @struct DebugText
 * @brief Debug text in 3D space or screen space.
 */
struct DebugText {
    UString text;
    Vector3D position;  // World position or screen position (if screenSpace is true)
    Color color;
    float duration;
    bool screenSpace;   // If true, position is in screen space
    float scale;

    KL_BEGIN_FIELDS(DebugText)
        KL_FIELD(DebugText, text, "Text", 0, 0),
        KL_FIELD(DebugText, position, "Position", -2147483648, 2147483647),
        KL_FIELD(DebugText, color, "Color", 0, 0),
        KL_FIELD(DebugText, duration, "Duration", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(DebugText, screenSpace, "Screen space", 0, 1),
        KL_FIELD(DebugText, scale, "Scale", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(DebugText)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(DebugText)
        /* No reflected ctors. */
    KL_END_DESCRIBE(DebugText)

};

/**
 * @class DebugDraw
 * @brief Central system for debug visualization.
 */
class DebugDraw {
private:
    // Singleton instance
    static DebugDraw* instance;

    // Debug primitives
    std::vector<DebugLine> lines;
    std::vector<DebugSphere> spheres;
    std::vector<DebugBox> boxes;
    std::vector<DebugText> texts;

    // Enabled state
    bool enabled;

    DebugDraw();

public:
    /**
     * @brief Gets the singleton instance.
     */
    static DebugDraw& GetInstance();

    /**
     * @brief Enables debug drawing.
     */
    void Enable() { enabled = true; }

    /**
     * @brief Disables debug drawing.
     */
    void Disable() { enabled = false; }

    /**
     * @brief Checks if debug drawing is enabled.
     */
    bool IsEnabled() const { return enabled; }

    /**
     * @brief Updates debug draws (removes expired ones).
     */
    void Update();

    /**
     * @brief Clears all debug draws.
     */
    void Clear();

    // === Line Drawing ===

    /**
     * @brief Draws a line.
     * @param start Start position.
     * @param end End position.
     * @param color Line color.
     * @param duration Duration in seconds (0 = one frame).
     * @param depthTest If false, draws on top of geometry.
     */
    void DrawLine(const Vector3D& start, const Vector3D& end, const Color& color = Color::White,
                  float duration = 0.0f, bool depthTest = true);

    /**
     * @brief Draws a ray (line from origin in direction).
     * @param origin Ray origin.
     * @param direction Ray direction (will be normalized).
     * @param length Ray length.
     * @param color Ray color.
     * @param duration Duration in seconds.
     * @param depthTest If false, draws on top of geometry.
     */
    void DrawRay(const Vector3D& origin, const Vector3D& direction, float length = 10.0f,
                 const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /**
     * @brief Draws an arrow (line with arrowhead).
     * @param start Start position.
     * @param end End position.
     * @param color Arrow color.
     * @param duration Duration in seconds.
     * @param depthTest If false, draws on top of geometry.
     */
    void DrawArrow(const Vector3D& start, const Vector3D& end, const Color& color = Color::White,
                   float duration = 0.0f, bool depthTest = true);

    // === Shape Drawing ===

    /**
     * @brief Draws a sphere.
     * @param center Sphere center.
     * @param radius Sphere radius.
     * @param color Sphere color.
     * @param duration Duration in seconds.
     * @param depthTest If false, draws on top of geometry.
     * @param mode Wireframe or solid.
     */
    void DrawSphere(const Vector3D& center, float radius, const Color& color = Color::White,
                    float duration = 0.0f, bool depthTest = true,
                    DebugDrawMode mode = DebugDrawMode::Wireframe);

    /**
     * @brief Draws an axis-aligned box.
     * @param center Box center.
     * @param extents Box half-extents.
     * @param color Box color.
     * @param duration Duration in seconds.
     * @param depthTest If false, draws on top of geometry.
     * @param mode Wireframe or solid.
     */
    void DrawBox(const Vector3D& center, const Vector3D& extents, const Color& color = Color::White,
                 float duration = 0.0f, bool depthTest = true,
                 DebugDrawMode mode = DebugDrawMode::Wireframe);

    /**
     * @brief Draws an oriented box.
     * @param center Box center.
     * @param extents Box half-extents.
     * @param transform Transformation matrix for orientation.
     * @param color Box color.
     * @param duration Duration in seconds.
     * @param depthTest If false, draws on top of geometry.
     * @param mode Wireframe or solid.
     */
    void DrawOrientedBox(const Vector3D& center, const Vector3D& extents, const Matrix4x4& transform,
                         const Color& color = Color::White, float duration = 0.0f, bool depthTest = true,
                         DebugDrawMode mode = DebugDrawMode::Wireframe);

    /**
     * @brief Draws a wireframe cube (unit cube centered at origin).
     * @param center Cube center.
     * @param size Cube size.
     * @param color Cube color.
     * @param duration Duration in seconds.
     * @param depthTest If false, draws on top of geometry.
     */
    void DrawCube(const Vector3D& center, float size, const Color& color = Color::White,
                  float duration = 0.0f, bool depthTest = true);

    // === Coordinate Systems ===

    /**
     * @brief Draws coordinate axes at a position.
     * @param position Axes origin.
     * @param scale Axes length.
     * @param duration Duration in seconds.
     * @param depthTest If false, draws on top of geometry.
     */
    void DrawAxes(const Vector3D& position, float scale = 1.0f, float duration = 0.0f, bool depthTest = true);

    /**
     * @brief Draws a grid on the XZ plane.
     * @param center Grid center.
     * @param size Grid size.
     * @param divisions Number of divisions.
     * @param color Grid color.
     * @param duration Duration in seconds.
     * @param depthTest If false, draws on top of geometry.
     */
    void DrawGrid(const Vector3D& center, float size, int divisions = 10,
                  const Color& color = Color::Gray, float duration = 0.0f, bool depthTest = true);

    // === Text Drawing ===

    /**
     * @brief Draws text in world space.
     * @param text The text to draw.
     * @param position World position.
     * @param color Text color.
     * @param duration Duration in seconds.
     * @param scale Text scale.
     */
    void DrawText(const UString& text, const Vector3D& position, const Color& color = Color::White,
                  float duration = 0.0f, float scale = 1.0f);

    /**
     * @brief Draws text in screen space.
     * @param text The text to draw.
     * @param screenX Screen X position (0-1).
     * @param screenY Screen Y position (0-1).
     * @param color Text color.
     * @param duration Duration in seconds.
     * @param scale Text scale.
     */
    void DrawScreenText(const UString& text, float screenX, float screenY,
                        const Color& color = Color::White, float duration = 0.0f, float scale = 1.0f);

    // === Accessors for Rendering ===

    /**
     * @brief Gets all debug lines.
     */
    const std::vector<DebugLine>& GetLines() const { return lines; }

    /**
     * @brief Gets all debug spheres.
     */
    const std::vector<DebugSphere>& GetSpheres() const { return spheres; }

    /**
     * @brief Gets all debug boxes.
     */
    const std::vector<DebugBox>& GetBoxes() const { return boxes; }

    /**
     * @brief Gets all debug texts.
     */
    const std::vector<DebugText>& GetTexts() const { return texts; }

    KL_BEGIN_FIELDS(DebugDraw)
        KL_FIELD(DebugDraw, enabled, "Enabled", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(DebugDraw)
        KL_METHOD_AUTO(DebugDraw, Enable, "Enable"),
        KL_METHOD_AUTO(DebugDraw, Disable, "Disable"),
        KL_METHOD_AUTO(DebugDraw, IsEnabled, "Is enabled"),
        KL_METHOD_AUTO(DebugDraw, Update, "Update"),
        KL_METHOD_AUTO(DebugDraw, Clear, "Clear"),
        KL_METHOD_AUTO(DebugDraw, DrawLine, "Draw line"),
        KL_METHOD_AUTO(DebugDraw, DrawSphere, "Draw sphere"),
        KL_METHOD_AUTO(DebugDraw, DrawBox, "Draw box"),
        KL_METHOD_AUTO(DebugDraw, DrawAxes, "Draw axes"),
        KL_METHOD_AUTO(DebugDraw, DrawGrid, "Draw grid"),
        KL_METHOD_AUTO(DebugDraw, DrawText, "Draw text")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(DebugDraw)
        // Singleton, no constructors exposed
    KL_END_DESCRIBE(DebugDraw)
};

} // namespace koilo
