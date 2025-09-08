#pragma once

#include <type_traits>
#include "../../../core/color/rgbcolor.hpp"
#include "../../../core/math/vector2d.hpp"
#include "../../../core/math/vector3d.hpp"

/**
 * @file ishader.hpp
 * @brief Stateless shader interface and per-fragment surface payload.
 */

//  Geometry payload for each fragment / sample
/**
 * @struct SurfaceProperties
 * @brief Geometry inputs supplied to shading for a single sample.
 *
 * All members are references; they must refer to valid vectors that outlive
 * the SurfaceProperties instance.
 */
struct SurfaceProperties {
    const Vector3D& position;   ///< Position in object or world space
    const Vector3D& normal;     ///< Interpolated normal (same space as lighting)
    const Vector3D& uvw;        ///< UV or barycentric coordinates

    /**
     * @brief Construct from explicit position/normal/uvw references.
     */
    SurfaceProperties(const Vector3D& p,
                      const Vector3D& n,
                      const Vector3D& t)
        : position(p), normal(n), uvw(t) {}

    SurfaceProperties(Vector3D&&, const Vector3D&, const Vector3D&) = delete;
    SurfaceProperties(const Vector3D&, Vector3D&&, const Vector3D&) = delete;
    SurfaceProperties(const Vector3D&, const Vector3D&, Vector3D&&) = delete;

    /**
     * @brief Helper to create from 2D UV (w = 0).
     * @param p  Position reference.
     * @param n  Normal reference.
     * @param uv 2D UV; Z is taken as 0.
     * @return SurfaceProperties holding references to inputs.
     */
    static SurfaceProperties FromUV(const Vector3D& p,
                                    const Vector3D& n,
                                    const Vector2D& uv) {
        const Vector3D& uvw_storage = Vector3D(uv.X, uv.Y, 0.0f);
        return SurfaceProperties(p, n, uvw_storage);
    }
};

//  Forward declarations to break include cycles
class IMaterial;

/**
 * @class IShader
 * @brief Abstract, stateless, shareable shader interface.
 */
class IShader {
public:
    virtual ~IShader() = default;

    /**
     * @brief Shade a single surface point.
     * @param surf Geometry inputs supplied by rasteriser / ray-hit.
     * @param mat  Parameter provider (concrete material).
     * @return Linear-space RGB colour.
     */
    virtual RGBColor Shade(const SurfaceProperties& surf,
                           const IMaterial&         mat) const = 0;
};
