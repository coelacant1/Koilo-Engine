#pragma once

#include "../ishader.hpp"
#include "../../material/materialt.hpp"    // for IMaterial::As<T>()
#include "combineparams.hpp"

#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

/**
 * @file combineshader.hpp
 * @brief Templated combiner shader that blends child materials using per-layer method and opacity.
 *
 * Each layer i evaluates its child material and blends into an accumulated RGB working color
 * (0..255 float domain) according to @ref CombineParams<N>::Method and @ref CombineParams<N>::opacity[i].
 *
 * @tparam N Maximum number of layers.
 */

/**
 * @class CombineShaderT
 * @brief Shader that samples N child materials and blends by method/opacity.
 *
 * The owning material is expected to be @c MaterialT<CombineParams<N>, CombineShaderT<N>> (or derivative).
 * This shader accesses the parameter block via @ref IMaterial::As<T>() to retrieve:
 * - @c materials[i] : child material pointers
 * - @c method[i]    : blend method
 * - @c opacity[i]   : blend amount in [0..1]
 * - @c count        : active layer count
 */
template <size_t N>
class CombineShaderT final : public IShader {
public:
    /**
     * @brief Shade a surface by accumulating/blending child material results.
     * @param surf Surface properties provided by the renderer.
     * @param m    Owning material; expected to expose @c CombineParams<N> via @ref IMaterial::As<T>().
     * @return Blended color in 8-bit RGB.
     */
    RGBColor Shade(const SurfaceProperties& surf, const IMaterial& m) const override {
        using BaseMat = MaterialT<CombineParams<N>, CombineShaderT<N>>;
        const auto& P = m.As<BaseMat>();

        // Working color in 0..255 floats
        Vector3D rgb { 0.0f, 0.0f, 0.0f };

        for (uint8_t i = 0; i < P.count; ++i) {
            const float a = P.opacity[i];
            if (a <= 0.025f) continue;

            const IMaterial* child = P.materials[i];
            if (!child || !child->GetShader()) continue;

            RGBColor src = child->GetShader()->Shade(surf, *child);
            Vector3D s { float(src.R), float(src.G), float(src.B) };
            Vector3D t;

            switch (P.method[i]) {
                case CombineParams<N>::Method::Base: {
                    // base layer = src * a
                    rgb = s * a;
                } break;

                case CombineParams<N>::Method::Add: {
                    // lerp to rgb + src
                    t = Vector3D(rgb.X + s.X, rgb.Y + s.Y, rgb.Z + s.Z);
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams<N>::Method::Subtract: {
                    t = Vector3D(rgb.X - s.X, rgb.Y - s.Y, rgb.Z - s.Z);
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams<N>::Method::Multiply: {
                    t = Vector3D(rgb.X * s.X, rgb.Y * s.Y, rgb.Z * s.Z);
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams<N>::Method::Divide: {
                    auto safeDiv = [](float x, float y){ return y != 0.0f ? (x / y) : x; };
                    t = Vector3D(safeDiv(rgb.X, s.X), safeDiv(rgb.Y, s.Y), safeDiv(rgb.Z, s.Z));
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams<N>::Method::Darken: {
                    t = Vector3D::Min(s, rgb);
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams<N>::Method::Lighten: {
                    t = Vector3D::Max(s, rgb);
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams<N>::Method::Screen: {
                    // 1 - (1-a)(1-b)  (with 0..255 scale)
                    t.X = 255.0f - (255.0f - rgb.X) * (255.0f - s.X) / 255.0f;
                    t.Y = 255.0f - (255.0f - rgb.Y) * (255.0f - s.Y) / 255.0f;
                    t.Z = 255.0f - (255.0f - rgb.Z) * (255.0f - s.Z) / 255.0f;
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams<N>::Method::Overlay: {
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

                case CombineParams<N>::Method::SoftLight: {
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

                case CombineParams<N>::Method::Replace: {
                    t = s;
                    rgb = Vector3D::LERP(rgb, t, a);
                } break;

                case CombineParams<N>::Method::EfficientMask: {
                    if (src.R > 128 && src.G > 128 && src.B > 128) {
                        rgb = s * a;
                        // terminate early
                        i = P.count; // relies on for loop stopping
                    }
                } break;

                case CombineParams<N>::Method::Bypass: {
                    // no-op: evaluate child (already did) but ignore result
                } break;

                default: break;
            }
        }

        rgb = rgb.Constrain(0.0f, 255.0f);
        return RGBColor( uint8_t(rgb.X), uint8_t(rgb.Y), uint8_t(rgb.Z) );
    }
};
