#pragma once

#include "../ishader.hpp"
#include "../../material/materialt.hpp"     // for IMaterial::As<T>()
#include "depthparams.hpp"

#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

/**
 * @file depthshader.hpp
 * @brief Stateless shader mapping depth along an axis to a 1D gradient lookup.
 *
 * Samples a child gradient material by projecting the surface position onto a chosen axis,
 * mapping the result into [0,1], and feeding that as the gradient coordinate.
 */

/**
 * @class DepthShader
 * @brief Stateless shader that converts axial depth into a gradient sample.
 *
 * Expects the bound material to be a @c MaterialT<DepthParams, DepthShader> (or a type
 * publicly deriving from it) so that @c IMaterial::As<T>() yields access to @ref DepthParams.
 */
class DepthShader final : public IShader {
public:
    /**
     * @brief Shade by projecting position onto the configured axis and sampling the gradient.
     * @param surf Surface properties (position/normal/uvw from the renderer).
     * @param m    Bound material, expected to be compatible with @c MaterialT<DepthParams, DepthShader>.
     * @return RGB color sampled from the child gradient; black (0,0,0) if no gradient/shader.
     *
     * @details
     * Steps:
     * 1. Read @ref DepthParams::axis and pick the corresponding component of @p surf.position.
     * 2. Remap @f$[-\tfrac{depth}{2} + offset,\ +\tfrac{depth}{2} + offset]@f$ to @f$[0,1]@f$.
     * 3. Build a local @ref SurfaceProperties with @c position.x = @c t and delegate to the gradient’s shader.
     */
    RGBColor Shade(const SurfaceProperties& surf, const IMaterial& m) const override {
        using DepthMatBase = MaterialT<DepthParams, DepthShader>;
        const auto& P = m.As<DepthMatBase>();

        if (!P.gradient || !P.gradient->GetShader())
            return {0,0,0};

        // pick axis value
        float axisValue = 0.0f;
        switch (P.axis) {
            case DepthAxis::X: axisValue = surf.position.X; break;
            case DepthAxis::Y: axisValue = surf.position.Y; break;
            case DepthAxis::Z: axisValue = surf.position.Z; break;
        }

        // map [-depth/2 + offset, +depth/2 + offset] -> [0..1]
        const float t = Mathematics::Map(
            axisValue,
            -P.depth * 0.5f + P.offset,
            +P.depth * 0.5f + P.offset,
            0.0f, 1.0f
        );

        // sample gradient by feeding position.x = t (others 0)
        Vector3D posL { t, 0.0f, 0.0f };
        Vector3D nrmL { 0.0f, 0.0f, 1.0f };
        Vector3D uvwL { t, 0.0f, 0.0f };

        SurfaceProperties sp { posL, nrmL, uvwL };
        return P.gradient->GetShader()->Shade(sp, *P.gradient);
    }
};
