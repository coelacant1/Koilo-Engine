#pragma once

#include "../ishader.hpp"
#include "../../material/materialt.hpp"    // for IMaterial::As<T>()
#include "../../../../registry/reflect_macros.hpp"
#include "combineparams.hpp"

#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

/**
 * @file combineshader.hpp
 * @brief Runtime combiner shader that blends child materials using per-layer method and opacity.
 *
 * Each layer i evaluates its child material and blends into an accumulated RGB working color
 * (0..255 float domain) according to @ref CombineParams::Method and @ref CombineParams::opacities.
 */

/**
 * @class CombineShader
 * @brief Shader that samples child materials and blends by method/opacity.
 *
 * The owning material is expected to be @c MaterialT<CombineParams, CombineShader> (or derivative).
 * This shader accesses the parameter block via @ref IMaterial::As<T>() to retrieve:
 * - @c materials[i] : child material pointers
 * - @c method[i]    : blend method
 * - @c opacity[i]   : blend amount in [0..1]
 * - layer count derived from vector sizes
 */
class CombineShader final : public IShader {
public:
    /**
     * @brief Shade a surface by accumulating/blending child material results.
     * @param surf Surface properties provided by the renderer.
    * @param m    Owning material; expected to expose @c CombineParams via @ref IMaterial::As<T>().
     * @return Blended color in 8-bit RGB.
     */
    RGBColor Shade(const SurfaceProperties& surf, const IMaterial& m) const override {
        using BaseMat = MaterialT<CombineParams, CombineShader>;
        const auto& P = m.As<BaseMat>();

        // Working color in 0..255 floats
        Vector3D rgb { 0.0f, 0.0f, 0.0f };

        const std::size_t layerCount = P.LayerCount();
        for (std::size_t i = 0; i < layerCount; ++i) {
            const float a = (i < P.opacities.size()) ? P.opacities[i] : 0.0f;
            if (a <= 0.025f) continue;

            const IMaterial* child = (i < P.materials.size()) ? P.materials[i] : nullptr;
            if (!child || !child->GetShader()) continue;

            RGBColor src = child->GetShader()->Shade(surf, *child);
            Vector3D s { float(src.R), float(src.G), float(src.B) };
            Vector3D t;

            const CombineParams::Method method =
                (i < P.methods.size()) ? P.methods[i] : CombineParams::Method::Bypass;

            switch (method) {
                case CombineParams::Method::Base: {
                    // base layer = src * a
                    rgb = s * a;
                } break;

                case CombineParams::Method::Add: {
                    // lerp to rgb + src
                    t = Vector3D(rgb.X + s.X, rgb.Y + s.Y, rgb.Z + s.Z);
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams::Method::Subtract: {
                    t = Vector3D(rgb.X - s.X, rgb.Y - s.Y, rgb.Z - s.Z);
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams::Method::Multiply: {
                    t = Vector3D(rgb.X * s.X, rgb.Y * s.Y, rgb.Z * s.Z);
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams::Method::Divide: {
                    auto safeDiv = [](float x, float y){ return y != 0.0f ? (x / y) : x; };
                    t = Vector3D(safeDiv(rgb.X, s.X), safeDiv(rgb.Y, s.Y), safeDiv(rgb.Z, s.Z));
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams::Method::Darken: {
                    t = Vector3D::Min(s, rgb);
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams::Method::Lighten: {
                    t = Vector3D::Max(s, rgb);
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams::Method::Screen: {
                    // 1 - (1-a)(1-b)  (with 0..255 scale)
                    t.X = 255.0f - (255.0f - rgb.X) * (255.0f - s.X) / 255.0f;
                    t.Y = 255.0f - (255.0f - rgb.Y) * (255.0f - s.Y) / 255.0f;
                    t.Z = 255.0f - (255.0f - rgb.Z) * (255.0f - s.Z) / 255.0f;
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams::Method::Overlay: {
                    auto ov = [](float aC, float bC){
                        return (aC < 128.0f)
                            ? (2.0f * aC * bC / 255.0f)
                            : (255.0f - 2.0f * (255.0f - aC) * (255.0f - bC) / 255.0f);
                    };
                    t.X = ov(rgb.X, s.X);
                    t.Y = ov(rgb.Y, s.Y);
                    t.Z = ov(rgb.Z, s.Z);
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams::Method::SoftLight: {
                    // (1 - 2b)a^2 + 2ba   (0..255 domain)
                    auto sl = [](float aC, float bC){
                        const float A = aC / 255.0f, B = bC / 255.0f;
                        return 255.0f * ((1.0f - 2.0f*B)*A*A + 2.0f*B*A);
                    };
                    t.X = sl(rgb.X, s.X);
                    t.Y = sl(rgb.Y, s.Y);
                    t.Z = sl(rgb.Z, s.Z);
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams::Method::Replace: {
                    t = s;
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams::Method::EfficientMask: {
                    if (src.R > 128 && src.G > 128 && src.B > 128) {
                        rgb = s * a;
                        break;
                    }
                } break;

                case CombineParams::Method::Bypass: {
                    // no-op: evaluate child (already did) but ignore result
                } break;

                default: break;
            }

            if (method == CombineParams::Method::EfficientMask &&
                src.R > 128 && src.G > 128 && src.B > 128) {
                break;
            }
        }

        rgb = rgb.Constrain(0.0f, 255.0f);
        return RGBColor( uint8_t(rgb.X), uint8_t(rgb.Y), uint8_t(rgb.Z) );
    }

    PTX_BEGIN_FIELDS(CombineShader)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(CombineShader)
        PTX_METHOD_AUTO(CombineShader, Shade, "Shade")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(CombineShader)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(CombineShader)
};
