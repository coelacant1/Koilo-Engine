// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file debugrenderer.hpp
 * @brief Renders DebugDraw primitives and overlay text into pixel buffers.
 *
 * Provides Bresenham line rasterization, 3D-to-2D projection using
 * the camera's transform/projection, and bitmap font text blitting
 * via the Characters glyph table.
 *
 * Tag-based, no vtable - reflection-safe.
 *
 * @date 02/21/2026
 * @author Coela
 */

#pragma once

#include <koilo/core/color/color888.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/vector2d.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <koilo/core/math/mathematics.hpp>
#include <koilo/assets/font/characters.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <koilo/debug/debugdraw.hpp>

namespace koilo {

class CameraBase;
class Scene;
class PhysicsWorld;
class Rasterizer;
class IPixelGroup;

/**
 * @struct DebugOverlayStats
 * @brief Stats displayed by the debug overlay.
 */
struct DebugOverlayStats {
    float fps         = 0.0f;
    float frameTimeMs = 0.0f;
    int   meshCount   = 0;
    int   triCount    = 0;
    int   particleCount = 0;

    KL_BEGIN_FIELDS(DebugOverlayStats)
        KL_FIELD(DebugOverlayStats, fps, "FPS", 0, 100000),
        KL_FIELD(DebugOverlayStats, frameTimeMs, "Frame time ms", 0, 100000),
        KL_FIELD(DebugOverlayStats, meshCount, "Mesh count", 0, 100000),
        KL_FIELD(DebugOverlayStats, triCount, "Tri count", 0, 10000000),
        KL_FIELD(DebugOverlayStats, particleCount, "Particle count", 0, 10000000)
    KL_END_FIELDS

    KL_BEGIN_METHODS(DebugOverlayStats)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(DebugOverlayStats)
        KL_CTOR0(DebugOverlayStats)
    KL_END_DESCRIBE(DebugOverlayStats)
};

/**
 * @class DebugRenderer
 * @brief Renders debug primitives and overlay into pixel buffers.
 *
 * All methods are static. Call after Rasterizer::Rasterize() to overlay
 * debug visuals on top of the rendered frame.
 */
class DebugRenderer {
public:
    // --- High-level API ----------------------------------------

    static void Render(DebugDraw& dd, CameraBase* camera);

    static void Render(DebugDraw& dd, CameraBase* camera,
                       Color888* buffer, int w, int h,
                       const float* depthBuf = nullptr,
                       int depthW = 0, int depthH = 0,
                       bool renderLines = true);

    static void RenderWireframe(Scene* scene, CameraBase* camera,
                                Color888 color = Color888(0, 255, 0));

    static void RenderOverlay(Color888* buffer, int w, int h,
                              const DebugOverlayStats& stats,
                              Color888 color = Color888(0, 255, 0));

    // --- Sphere Wireframe --------------------------------------

    static void DrawSphere3D(Color888* buffer, int w, int h,
                             const Vector3D& center, float radius,
                             Color888 color,
                             const Vector3D& camPos,
                             const Quaternion& invRot,
                             const Vector3D& camScale,
                             bool persp, float fovScale, float nearPlane,
                             const Vector2D& minC, const Vector2D& maxC,
                             int segments = 16);

    // --- Physics Debug Visualization ---------------------------

    static void RenderPhysicsColliders(PhysicsWorld* world, CameraBase* camera,
                                       Color888 sphereColor = Color888(0, 200, 255),
                                       Color888 boxColor = Color888(255, 200, 0),
                                       Color888 capsuleColor = Color888(200, 0, 255));

    static void RenderPhysicsContacts(PhysicsWorld* world, CameraBase* camera,
                                       float normalLength = 2.0f,
                                       Color888 pointColor = Color888(255, 0, 0),
                                       Color888 normalColor = Color888(255, 255, 0));

    // --- Depth & Normal Visualization --------------------------

    static void RenderDepthView(Color888* buffer, int w, int h);
    static void RenderNormalView(Color888* buffer, int w, int h);

    // --- Low-level drawing primitives --------------------------

    static void DrawLine2D(Color888* buffer, int w, int h,
                           int x0, int y0, int x1, int y1,
                           Color888 color);

    static void DrawLine2DDepthTested(Color888* buffer, int w, int h,
                                       int x0, int y0, int x1, int y1,
                                       float z0, float z1,
                                       const float* depthBuf,
                                       Color888 color);

    static void BlitText(Color888* buffer, int w, int h,
                         int startX, int startY,
                         const char* text, Color888 color, int scale = 1);

    // --- Projection --------------------------------------------

    static bool ProjectLineClipped(const Vector3D& worldA, const Vector3D& worldB,
                                   const Vector3D& camPos,
                                   const Quaternion& invCamRot,
                                   const Vector3D& camScale,
                                   bool perspective, float fovScale,
                                   float nearPlane,
                                   const Vector2D& minC, const Vector2D& maxC,
                                   int w, int h,
                                   int& outX0, int& outY0,
                                   int& outX1, int& outY1,
                                   float& outZ0, float& outZ1);

    static bool ProjectPoint(const Vector3D& worldPos,
                             const Vector3D& camPos,
                             const Quaternion& invCamRot,
                             const Vector3D& camScale,
                             bool perspective, float fovScale,
                             float nearPlane,
                             const Vector2D& minC, const Vector2D& maxC,
                             int w, int h,
                             int& outX, int& outY);

private:
    static void RenderInto(DebugDraw& dd, CameraBase* camera,
                           Color888* buffer, int w, int h,
                           const Vector2D& minC, const Vector2D& maxC,
                           const float* depthBuf = nullptr,
                           int depthW = 0, int depthH = 0,
                           bool renderLines = true);

    static Color888 ColorToC888(const Color& c);

    static void RenderBox(Color888* buffer, int w, int h,
                          const DebugBox& box,
                          const Vector3D& camPos,
                          const Quaternion& invRot,
                          const Vector3D& camScale,
                          bool persp, float fovScale, float nearPlane,
                          const Vector2D& minC, const Vector2D& maxC);

    // === Reflection (static utility - no fields/instance methods) ===
    KL_DECLARE_FIELDS(DebugRenderer)
    KL_DECLARE_METHODS(DebugRenderer)
    KL_DECLARE_DESCRIBE(DebugRenderer)
};

} // namespace koilo
