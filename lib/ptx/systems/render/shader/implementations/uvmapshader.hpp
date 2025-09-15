// uvmapshader.hpp
#pragma once

#include "../ishader.hpp"
#include "../../material/materialt.hpp"   // for IMaterial::As<T>()
#include "uvmapparams.hpp"
#include "../../../../registry/reflect_macros.hpp"

#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

/**
 * @file uvmapshader.hpp
 * @brief Stateless shader that maps UVs to image color and applies a hue shift.
 */
class UVMapShader final : public IShader {
public:
    /**
     * @brief Sample the bound image at (u,v) and apply hue shift from parameters.
     * @param surf Surface properties providing UVW coordinates.
     * @param m    Material carrying a @ref UVMapParams block via @c MaterialT.
     * @return Sampled color after optional flips and hue shift; black if no image is bound.
     *
     * Expects the material to be a @c MaterialT<UVMapParams, UVMapShader>. Access to the
     * parameter block is obtained via @c IMaterial::As<>().
     */
    RGBColor Shade(const SurfaceProperties& surf, const IMaterial& m) const override {
        using MatBase = MaterialT<UVMapParams, UVMapShader>;
        const auto& p = m.As<MatBase>();
        if (!p.image) return {0,0,0};

        // Start with UVs from the surface
        float u = surf.uvw.X;
        float v = surf.uvw.Y;

        // Optional flips to match legacy behavior
        if (p.flipU) u = 1.0f - u;
        if (p.flipV) v = 1.0f - v;

        // Sample and hue-shift
        RGBColor c = p.image->GetColorAtCoordinate(Vector2D(u, v));
        return c.HueShift(p.hueAngle);
    }

    PTX_BEGIN_FIELDS(UVMapShader)
        /* TODO: PTX_FIELD(UVMapShader, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(UVMapShader)
        PTX_METHOD_AUTO(UVMapShader, Shade, "Shade")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(UVMapShader)
        /* TODO: PTX_CTOR0(UVMapShader) or PTX_CTOR(UVMapShader, ...) */
    PTX_END_DESCRIBE(UVMapShader)
    
};
