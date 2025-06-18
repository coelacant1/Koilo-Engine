/**
 * @file rastertriangle2d.hpp
 * @brief Triangle prepared for barycentric rasterization.
 * @date  18/06/2025
 */
#pragma once

#include "../../../../../include/um3d/geometry/2d/triangle.hpp"
#include "../../../../../include/um3d/math/vector2d.hpp"
#include "../../../../../include/um3d/math/vector3d.hpp"
#include "../../../../../include/um3d/render/material/material.hpp"

/**
 * @class RasterTriangle2D
 * @brief Pre-computes edge data for fast pixel tests; holds optional UV / depth.
 */
class RasterTriangle2D{
public:
    Triangle2D tri;             ///< geometric triangle
    float denom;                ///< 1 / denominator for barycentric
    float v0x, v0y;             ///< edge vectors
    float v1x, v1y;
    float v2x, v2y;
    const Vector2D* uv1;        ///< optional UV pointers
    const Vector2D* uv2;
    const Vector2D* uv3;
    const Vector3D* nrm;        ///< optional normal pointer
    Material* mat;              ///< material reference
    float avgDepth;             ///< average eye-space depth
    bool hasUV;                 ///< flag set if uv1 non-null

    /** @brief Build from analytic triangle only (no UV). */
    explicit RasterTriangle2D(const Triangle2D& t);

    /** @brief Full ctor with UV, normal, material. */
    RasterTriangle2D(const Triangle2D& t,
                     const Vector2D* uv1In,
                     const Vector2D* uv2In,
                     const Vector2D* uv3In,
                     const Vector3D* nrmIn,
                     Material* matIn,
                     float depthIn);

    /**
     * @brief Fast barycentric intersection test.
     * @param x screen-space x
     * @param y screen-space y
     * @param u out barycentric u
     * @param v out barycentric v
     * @param w out barycentric w
     * @return true if inside
     */
    bool Hit(float x, float y, float& u, float& v, float& w) const;
};
