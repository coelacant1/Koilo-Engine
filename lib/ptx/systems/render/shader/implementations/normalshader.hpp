// normalshader.hpp
#pragma once

#include "../ishader.hpp"
#include "../../material/materialt.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "normalparams.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file normalshader.hpp
 * @brief Shader that encodes the surface normal into RGB for visualization.
 *
 * The normal is normalized to unit length, remapped from [-1,1] to [0,255] per channel,
 * and clamped to byte range before packing into RGBColor.
 */
class NormalShader final : public IShader {
public:
    /**
     * @brief Shade using normalized surface normal encoded as RGB.
     * @param surf Surface properties (uses @c surf.normal).
     * @param m    Bound material (unused).
     * @return RGBColor where R,G,Z represent the remapped normal components.
     */
    RGBColor Shade(const SurfaceProperties& surf, const IMaterial& m) const override {
        (void)m;

        Vector3D n = Vector3D(surf.normal).UnitSphere();   // normalize
        n = (n + 1.0f) * 0.5f * 255.0f;                    // [-1,1] -> [0,1] -> [0,255]
        n = n.Constrain(0.0f, 255.0f);                     // clamp to byte range

        return RGBColor(uint8_t(n.X), uint8_t(n.Y), uint8_t(n.Z));
    }

    PTX_BEGIN_FIELDS(NormalShader)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(NormalShader)
        PTX_METHOD_AUTO(NormalShader, Shade, "Shade")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(NormalShader)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(NormalShader)

};
