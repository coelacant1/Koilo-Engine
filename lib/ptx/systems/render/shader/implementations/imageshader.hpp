#pragma once

#include "../ishader.hpp"
#include "../../material/materialt.hpp"

#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "imageparams.hpp"

/**
 * @file imageshader.hpp
 * @brief Stateless shader that samples a palette-indexed image and applies a hue shift.
 */

/**
 * @class ImageShader
 * @brief Stateless image shader.
 *
 * Chooses UV or position coordinates from @ref SurfaceProperties, samples the bound
 * image in @ref ImageParams via @ref Image::GetColorAtCoordinate, then applies a hue shift.
 */
class ImageShader final : public IShader {
public:
    /**
     * @brief Shade a surface point by sampling the material's image and hue-shifting the result.
     * @param surf Surface properties (position, uvw).
     * @param m    Material expected to be @c MaterialT<ImageParams, ImageShader>.
     * @return RGB color after sampling and hue shift; black if no image is bound.
     */
    RGBColor Shade(const SurfaceProperties& surf, const IMaterial& m) const override {
        using ImageMat = MaterialT<ImageParams, ImageShader>;
        const auto& p = m.As<ImageMat>();
        if (!p.image) return {0,0,0};

        // Choose coordinates
        const float u = p.useUV ? surf.uvw.X : surf.position.X;
        const float v = p.useUV ? surf.uvw.Y : surf.position.Y;

        // Sample and hue-shift
        RGBColor c = p.image->GetColorAtCoordinate(Vector2D(u, v));
        return c.HueShift(p.hueAngle);
    }
};
