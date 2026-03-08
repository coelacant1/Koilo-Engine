// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file rasterizer.h
 * @brief Provides functionality for rasterizing 3D scenes into 2D camera views.
 *
 * The Rasterizer class handles rendering a 3D scene by projecting it onto a 2D camera view.
 * It supports triangle-based rasterization with optional acceleration structures for efficiency.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't, moepforfreedom
 */

#pragma once

#include <koilo/systems/scene/scene.hpp>
#include <koilo/systems/scene/camera/camerabase.hpp>
#include <koilo/core/color/color888.hpp>
#include "helpers/rastertriangle2d.hpp"
#include <koilo/registry/reflect_macros.hpp>
#include <vector>


namespace koilo {

/**
 * @class Rasterizer
 * @brief Provides static methods for rasterizing 3D scenes into 2D camera views.
 */
class Rasterizer {
private:
    // Depth buffer - always populated after Rasterize(); used by DrawLine3D
    static std::vector<float> s_depthBuf;
    static int s_bufW, s_bufH;

    // Normal debug buffer (opt-in)
    static bool s_debugEnabled;
    static std::vector<float> s_normalBuf;

    // Projection state saved from the last Rasterize() call; consumed by DrawLine3D
    static bool s_rsPerspective;
    static float s_rsFovScale, s_rsNearPlane;
    static float s_rsCX, s_rsCY, s_rsHH;
    static Quaternion s_rsInvRot;
    static Vector3D s_rsCamPos, s_rsCamScl;

public:
    static void Rasterize(Scene* scene, CameraBase* camera);

    /**
     * @brief Rasterize a single world-space line segment into buffer using the
     *        depth buffer from the last Rasterize() call.
     *
     * The line is transformed, near-plane clipped, viewport-clipped (Liang-Barsky),
     * then Bresenham-drawn with per-pixel linear-Z depth testing that exactly
     * matches the triangle rasterizer's depth convention.
     *
     * @param worldA  World-space start point.
     * @param worldB  World-space end point.
     * @param color   Line color.
     * @param depthTest  If true, tests and writes the depth buffer.
     * @param buffer  Flat Color888 output (row-major, size w*h).
     * @param w       Buffer width.
     * @param h       Buffer height.
     */
    static void DrawLine3D(const Vector3D& worldA, const Vector3D& worldB,
                           Color888 color, bool depthTest,
                           Color888* buffer, int w, int h);

    // --- Debug buffer control ---
    static void EnableDebugBuffers(bool enable) { s_debugEnabled = enable; }
    static bool DebugBuffersEnabled() { return s_debugEnabled; }
    static const float* GetDepthBuffer() { return s_depthBuf.empty() ? nullptr : s_depthBuf.data(); }
    static const float* GetNormalBuffer() { return s_normalBuf.empty() ? nullptr : s_normalBuf.data(); }
    static int GetDebugWidth() { return s_bufW; }
    static int GetDebugHeight() { return s_bufH; }

    KL_BEGIN_FIELDS(Rasterizer)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Rasterizer)
        KL_SMETHOD_AUTO(Rasterizer::Rasterize, "Rasterize")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Rasterizer)
        /* No reflected ctors. */
    KL_END_DESCRIBE(Rasterizer)

};

} // namespace koilo