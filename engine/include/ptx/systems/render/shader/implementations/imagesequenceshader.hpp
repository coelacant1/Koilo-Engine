#pragma once

#include "../ishader.hpp"
#include "../../material/materialt.hpp"   // for IMaterial::As<T>()
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "imagesequenceparams.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file imagesequenceshader.hpp
 * @brief Stateless shader that samples the current frame of an image sequence and applies a hue shift.
 */

/**
 * @class ImageSequenceShader
 * @brief Samples the bound ImageSequence at the requested coordinate (UV or world) and hue-shifts the result.
 *
 * Expects the bound material to be a @c MaterialT<ImageSequenceParams, ImageSequenceShader>.
 * If no sequence is bound, returns black.
 */
class ImageSequenceShader final : public IShader {
public:
    /**
     * @brief Shade by sampling the current sequence frame and applying a hue shift.
     * @param surf Surface properties (uses @c surf.uvw if @c useUV is true, otherwise @c surf.position).
     * @param m    Material bound at the call site; must be @c MaterialT<ImageSequenceParams, ImageSequenceShader>.
     * @return Hue-shifted @c RGBColor; black if @c sequence is null.
     */
    RGBColor Shade(const SurfaceProperties& surf, const IMaterial& m) const override {
        using SeqMatBase = MaterialT<ImageSequenceParams, ImageSequenceShader>;
        const auto& p = m.As<SeqMatBase>();
        if (!p.sequence) return {0,0,0};

        const float u = p.useUV ? surf.uvw.X : surf.position.X;
        const float v = p.useUV ? surf.uvw.Y : surf.position.Y;

        RGBColor c = p.sequence->GetColorAtCoordinate(Vector2D(u, v));
        return c.HueShift(p.hueAngle);
    }

    PTX_BEGIN_FIELDS(ImageSequenceShader)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(ImageSequenceShader)
        PTX_METHOD_AUTO(ImageSequenceShader, Shade, "Shade")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(ImageSequenceShader)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(ImageSequenceShader)

};
