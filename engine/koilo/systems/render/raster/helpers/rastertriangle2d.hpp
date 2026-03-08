// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file rastertriangle2d.hpp
 * @brief Flat rasterization triangle - no inheritance, cache-optimised layout.
 * @date  26/06/2025
 * @author Coela Can't
 */
#pragma once

#include <koilo/core/math/transform.hpp>
#include <koilo/core/math/vector2d.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/systems/render/material/imaterial.hpp>
#include <koilo/ksl/ksl_shader.hpp>
#include "rastertriangle3d.hpp"
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @struct RasterTriangle2D
 * @brief Flat POD-like struct for rasterization - no vtable, no inheritance.
 *
 * Holds projected 2D screen vertices, inline world-space positions/UVs,
 * and precomputed barycentric + AABB data.  All data lives in one
 * contiguous block for cache locality in the per-pixel inner loop.
 */
struct RasterTriangle2D {
    // --- Screen-space vertices (projected 2D) ---
    Vector2D p1, p2, p3;

    // --- World-space vertex positions (inline, no pointer chase) ---
    Vector3D wp1, wp2, wp3;

    // --- Surface normal ---
    Vector3D normal;

    // --- Material (kept for reflection/scripting) ---
    IMaterial* material;

    // --- Direct shade pointers (hot-path, skip virtual dispatch) ---
    ksl::KSLShadeFn shadeFn = nullptr;
    void* shaderInstance = nullptr;
    const ksl::FrameContext* frameCtx = nullptr;
    uint8_t attribMask = ksl::SHADE_ATTRIB_ALL;  ///< Which ShadeInput fields this shader reads

    // --- UV coordinates (inline; default (0,0) when absent) ---
    Vector2D uv1, uv2, uv3;

    // --- Per-vertex projected Z depths ---
    float z1, z2, z3;

    // --- Precomputed barycentric data ---
    float denominator;       ///< 1/det for barycentric coordinate calculations.
    Vector2D v0, v1;         ///< Edge vectors for barycentric calculations.

    // --- AABB as raw floats (no Rectangle2D overhead) ---
    float boundsMinX, boundsMinY, boundsMaxX, boundsMaxY;

    // --- Precomputed pixel-space AABB (avoids float->int in scanline loop) ---
    int16_t pixMinX, pixMinY, pixMaxX, pixMaxY;

    // --- Depth sort key ---
    float averageDepth;

    // --- Constructors ---
    RasterTriangle2D();

    RasterTriangle2D(const Transform& camTransform, const Quaternion& lookDirection,
                     const RasterTriangle3D& sourceTriangle, IMaterial* mat);

    RasterTriangle2D(const Transform& camTransform, const Quaternion& lookDirection,
                     const RasterTriangle3D& sourceTriangle, IMaterial* mat,
                     float nearPlane, float fovScale,
                     float viewportCenterX, float viewportCenterY,
                     float viewportHalfW, float viewportHalfH);

    /** @brief Direct construction from raw vertex/UV pointers (orthographic). */
    RasterTriangle2D(const Transform& camTransform, const Quaternion& lookDirection,
                     const Vector3D* v1, const Vector3D* v2, const Vector3D* v3,
                     const Vector2D* t1, const Vector2D* t2, const Vector2D* t3,
                     IMaterial* mat);

    /** @brief Direct construction from raw vertex/UV pointers (perspective). */
    RasterTriangle2D(const Transform& camTransform, const Quaternion& lookDirection,
                     const Vector3D* v1, const Vector3D* v2, const Vector3D* v3,
                     const Vector2D* t1, const Vector2D* t2, const Vector2D* t3,
                     IMaterial* mat,
                     float nearPlane, float fovScale,
                     float viewportCenterX, float viewportCenterY,
                     float viewportHalfW, float viewportHalfH);

    bool IsBackFacing() const;

    bool GetBarycentricCoords(float x, float y, float& u, float& v, float& w) const;

    /** @brief Simple AABB overlap test. */
    bool Overlaps(float oMinX, float oMinY, float oMaxX, float oMaxY) const;

    IMaterial* GetMaterial() const;

    koilo::UString ToString() const;

private:
    void CalculateBoundsAndDenominator();

    KL_BEGIN_FIELDS(RasterTriangle2D)
        KL_FIELD(RasterTriangle2D, wp1, "Wp1", 0, 0),
        KL_FIELD(RasterTriangle2D, wp2, "Wp2", 0, 0),
        KL_FIELD(RasterTriangle2D, wp3, "Wp3", 0, 0),
        KL_FIELD(RasterTriangle2D, normal, "Normal", 0, 0),
        KL_FIELD(RasterTriangle2D, material, "Material", 0, 0),
        KL_FIELD(RasterTriangle2D, uv1, "Uv1", 0, 0),
        KL_FIELD(RasterTriangle2D, uv2, "Uv2", 0, 0),
        KL_FIELD(RasterTriangle2D, uv3, "Uv3", 0, 0),
        KL_FIELD(RasterTriangle2D, averageDepth, "Average depth", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(RasterTriangle2D, denominator, "Denominator", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(RasterTriangle2D, v0, "V0", 0, 0),
        KL_FIELD(RasterTriangle2D, v1, "V1", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(RasterTriangle2D)
        KL_METHOD_AUTO(RasterTriangle2D, GetBarycentricCoords, "Get barycentric coords"),
        KL_METHOD_AUTO(RasterTriangle2D, GetMaterial, "Get material"),
        KL_METHOD_AUTO(RasterTriangle2D, ToString, "To string")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(RasterTriangle2D)
        KL_CTOR0(RasterTriangle2D),
        KL_CTOR(RasterTriangle2D, const Transform &, const Quaternion &, const RasterTriangle3D &, IMaterial *)
    KL_END_DESCRIBE(RasterTriangle2D)

};

} // namespace koilo
