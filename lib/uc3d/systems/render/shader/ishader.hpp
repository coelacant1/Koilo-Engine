#pragma once

#include <type_traits>
#include "../../../core/color/rgbcolor.hpp"
#include "../../../core/math/vector3d.hpp"

//  Geometry payload for each fragment / sample
struct SurfaceProperties {
    const Vector3D& position;   ///< Position in object or world space
    const Vector3D& normal;     ///< Interpolated normal (same space as lighting)
    const Vector3D& uvw;        ///< UV or barycentric coordinates
};

//  Forward declarations to break include cycles
class IMaterial;

//  IShader - ABSTRACT, stateless, shareable
class IShader {
public:
    virtual ~IShader() = default;

    /**
     * @brief Shade a single surface point.
     * @param surf  Geometry inputs supplied by rasteriser / ray‑hit.
     * @param mat   Parameter provider (concrete material).
     * @return      Linear‑space RGB colour.
     */
    virtual RGBColor Shade(const SurfaceProperties& surf,
                           const IMaterial&         mat) const = 0;
};
