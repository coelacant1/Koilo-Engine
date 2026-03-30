// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file debug_visualization.hpp
 * @brief CVar-driven debug overlays for physics colliders, scene bounds,
 *        and camera frustum visualization.
 *
 * Submits wireframe primitives to DebugDraw each frame when enabled.
 * Works with both GPU (RHI debug line pipeline) and SW (DebugRenderer)
 * rendering paths since all geometry is emitted as DebugDraw lines.
 */

#pragma once

#include <koilo/kernel/cvar/cvar_system.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/debug/debugdraw.hpp>

namespace koilo {

class PhysicsWorld;
class Scene;
class CameraBase;

/// Submit debug visualization lines based on active CVars.
/// Call once per frame before debug line rendering.
class DebugVisualization {
public:
    /// Submit all enabled debug overlays for this frame.
    /// Each call emits zero-duration (single-frame) lines to DebugDraw.
    static void Submit(PhysicsWorld* physics, Scene* scene,
                       CameraBase* camera);

    /// Draw wireframe AABBs, spheres, and capsules for all colliders.
    static void DrawPhysicsColliders(PhysicsWorld* world);

    /// Draw contact points as small crosses and penetration normals.
    static void DrawPhysicsContacts(PhysicsWorld* world);

    /// Draw per-mesh AABB wireframes for all scene meshes.
    static void DrawSceneBounds(Scene* scene);

    /// Draw the camera frustum as a wireframe.
    static void DrawCameraFrustum(CameraBase* camera,
                                  float aspectRatio);

    /// Draw a wireframe box from min/max corners.
    static void DrawWireBox(const Vector3D& mn, const Vector3D& mx,
                            const Color& color);

    /// Draw a wireframe sphere as 3 orthogonal circle rings.
    static void DrawWireSphere(const Vector3D& center, float radius,
                               const Color& color, int segments = 16);

    /// Draw a wireframe capsule (two hemisphere rings + connecting lines).
    static void DrawWireCapsule(const Vector3D& p1, const Vector3D& p2,
                                float radius, const Color& color,
                                int segments = 12);

    /// Draw a small cross marker at a point.
    static void DrawCross(const Vector3D& pos, float size,
                          const Color& color);
};

} // namespace koilo
