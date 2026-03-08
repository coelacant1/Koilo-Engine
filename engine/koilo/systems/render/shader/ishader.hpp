// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <koilo/core/color/color888.hpp>
#include <koilo/core/math/vector2d.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

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
    Vector3D viewDirection;     ///< Normalized camera->surface direction
    Vector3D tangent;           ///< Surface tangent (for normal mapping)
    Vector3D bitangent;         ///< Surface bitangent (for normal mapping)

    /**
     * @brief Construct from explicit position/normal/uvw references.
     */
    SurfaceProperties(const Vector3D& p,
                      const Vector3D& n,
                      const Vector3D& t)
        : position(p), normal(n), uvw(t),
          viewDirection(0, 0, 0), tangent(0, 0, 0), bitangent(0, 0, 0) {}

    /**
     * @brief Full construction with view direction and TBN basis.
     */
    SurfaceProperties(const Vector3D& p,
                      const Vector3D& n,
                      const Vector3D& t,
                      const Vector3D& viewDir,
                      const Vector3D& tan,
                      const Vector3D& bitan)
        : position(p), normal(n), uvw(t),
          viewDirection(viewDir), tangent(tan), bitangent(bitan) {}

    SurfaceProperties(Vector3D&&, const Vector3D&, const Vector3D&) = delete;
    SurfaceProperties(const Vector3D&, Vector3D&&, const Vector3D&) = delete;
    SurfaceProperties(const Vector3D&, const Vector3D&, Vector3D&&) = delete;

    KL_BEGIN_FIELDS(SurfaceProperties)
        KL_FIELD(SurfaceProperties, position, "Position", 0, 0),
        KL_FIELD(SurfaceProperties, normal, "Normal", 0, 0),
        KL_FIELD(SurfaceProperties, uvw, "Uvw", 0, 0),
        KL_FIELD(SurfaceProperties, viewDirection, "View direction", 0, 0),
        KL_FIELD(SurfaceProperties, tangent, "Tangent", 0, 0),
        KL_FIELD(SurfaceProperties, bitangent, "Bitangent", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(SurfaceProperties)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(SurfaceProperties)
        KL_CTOR(SurfaceProperties, const Vector3D &, const Vector3D &, const Vector3D &)
    KL_END_DESCRIBE(SurfaceProperties)

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
    virtual Color888 Shade(const SurfaceProperties& surf,
                           const IMaterial&         mat) const = 0;

};

} // namespace koilo
