// vectorfield2dshader.hpp
#pragma once
#include <cstdint>
#include <cmath>
#include "../../../../registry/reflect_macros.hpp"

#include "../../shader/ishader.hpp"
#include "../../material/imaterial.hpp"
#include "../../material/materialt.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "vectorfield2dparams.hpp"

/**
 * @file vectorfield2dshader.hpp
 * @brief Shader that samples a VectorField2DParams density grid with bilinear filtering.
 *
 * Interprets @ref SurfaceProperties::uvw as normalized local coordinates, scales by
 * @ref VectorField2DParams::size, applies rotation about the field center, and samples
 * the current density channel @ref VectorField2DParams::vecD. The density is mapped to
 * a simple blue<->red ramp.
 */
class VectorField2DShader final : public IShader {
public:
    /**
     * @brief Shade a point by sampling the vector field’s density.
     * @param sp Surface properties (uses @c sp.uvw.X/Y).
     * @param m  Material expected to be @c MaterialT<VectorField2DParams, VectorField2DShader>.
     * @return RGB color mapped from sampled density (blue->red). Returns black if out-of-bounds.
     */
    RGBColor Shade(const SurfaceProperties& sp, const IMaterial& m) const override {
        using VFieldMat = MaterialT<VectorField2DParams, VectorField2DShader>;

        // MaterialT publicly mixes in VectorField2DParams; access parameters via typed view.
        const auto& p = m.As<VFieldMat>();

        const float px = sp.uvw.X * p.size.X;
        const float py = sp.uvw.Y * p.size.Y;

        auto sampleDensity = [&](float x, float y, bool& inBounds) -> std::uint32_t {
            Vector2D input(x, y);
            input = input - p.position + p.size / 2.0f;
            if (!Mathematics::IsClose(p.rotation, 0.0f, 0.001f)) {
                input = input.Rotate(p.rotation, p.size / 2.0f);
            }
            inBounds = (input.X > 0 && input.X < p.size.X &&
                        input.Y > 0 && input.Y < p.size.Y);
            if (!inBounds) return 0;

            const float sx = Mathematics::Map(input.X, 0.0f, p.size.X, 0.0f, float(p.countX - 1));
            const float sy = Mathematics::Map(input.Y, 0.0f, p.size.Y, 0.0f, float(p.countY - 1));

            // Clamp so (cx+1, ry+1) remain valid.
            const std::uint16_t cx = (std::uint16_t)std::floor(std::fmin(std::fmax(sx, 0.0f), float(p.countX - 2)));
            const std::uint16_t ry = (std::uint16_t)std::floor(std::fmin(std::fmax(sy, 0.0f), float(p.countY - 2)));

            const std::uint32_t q11 = ry * p.countX + cx;
            const std::uint32_t q12 = ry * p.countX + (cx + 1);
            const std::uint32_t q21 = (ry + 1) * p.countX + cx;
            const std::uint32_t q22 = (ry + 1) * p.countX + (cx + 1);

            const float x1 =  cx       * (p.size.X / float(p.countX - 1));
            const float x2 = (cx + 1)  * (p.size.X / float(p.countX - 1));
            const float y1 =  ry       * (p.size.Y / float(p.countY - 1));
            const float y2 = (ry + 1)  * (p.size.Y / float(p.countY - 1));

            const float v11 = float(p.vecD[q11]);
            const float v12 = float(p.vecD[q12]);
            const float v21 = float(p.vecD[q21]);
            const float v22 = float(p.vecD[q22]);

            const float v = Mathematics::BilinearInterpolation(sx, sy, x1, y1, x2, y2,
                                                               v11, v12, v21, v22);
            return std::uint32_t(v);
        };

        bool inBounds = false;
        const std::uint32_t d = sampleDensity(px, py, inBounds);
        if (!inBounds) return RGBColor(0,0,0);

        const float r = float(d) / 127.0f;
        return RGBColor(r, 0.0f, 1.0f - r);
    }

    PTX_BEGIN_FIELDS(VectorField2DShader)
        /* TODO: PTX_FIELD(VectorField2DShader, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(VectorField2DShader)
        PTX_METHOD_AUTO(VectorField2DShader, Shade, "Shade")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(VectorField2DShader)
        /* TODO: PTX_CTOR0(VectorField2DShader) or PTX_CTOR(VectorField2DShader, ...) */
    PTX_END_DESCRIBE(VectorField2DShader)
    
};
