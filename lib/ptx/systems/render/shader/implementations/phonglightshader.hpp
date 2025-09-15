// phonglightshader.hpp
#pragma once

#include <cstddef>
#include <cmath>
#include "../../../../registry/reflect_macros.hpp"

#include "../ishader.hpp"
#include "../../material/materialt.hpp"
#include "phonglightparams.hpp"

#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

/**
 * @file phonglightshader.hpp
 * @brief Stateless Phong lighting shader with N lights and distance attenuation.
 * @tparam lightCount Number of lights compiled into the material.
 *
 * Uses the Light API (position, intensity, falloff, curveA, curveB).
 */
template <size_t lightCount>
class PhongLightShaderT final : public IShader {
public:
    /**
     * @brief Shade a surface using Phong lighting with distance attenuation.
     * @param surf Surface properties (position, normal, etc.).
     * @param m    Material providing @ref PhongLightParams for @p lightCount lights.
     * @return Lit color in RGB.
     */
    RGBColor Shade(const SurfaceProperties& surf, const IMaterial& m) const override {
        using MatBase = MaterialT<PhongLightParams<lightCount>, PhongLightShaderT<lightCount>>;
        const auto& P = m.As<MatBase>();

        // Normalize surface normal and compute view direction
        Vector3D N = Vector3D(surf.normal).UnitSphere();
        Vector3D V = Vector3D(P.cameraPos - surf.position).UnitSphere();

        // Ambient term accumulator (start with ambient contribution as vector)
        Vector3D accum(
            float(P.ambientColor.R),
            float(P.ambientColor.G),
            float(P.ambientColor.B)
        );

        for (size_t i = 0; i < lightCount; ++i) {
            Light& Lgt = P.lights[i];

            // Light direction and distance
            Vector3D Ldir = Vector3D(Lgt.GetPosition() - surf.position);
            const float dist = Ldir.Magnitude();
            if (dist <= 0.0001f) continue;
            Ldir = Ldir / dist;

            // Lambert diffuse factor
            const float NdotL = Mathematics::Max(N.DotProduct(Ldir), 0.0f);
            if (NdotL <= 0.0f) continue;

            // Distance attenuation: 1 / (1 + a*d + b*(d/f)^2)
            const float falloff = Lgt.GetFalloff();
            const float curveA  = Lgt.GetCurveA();
            const float curveB  = Lgt.GetCurveB();
            const float att     = 1.0f / (1.0f + curveA * dist + curveB * std::pow(dist / (falloff > 0.0f ? falloff : 1.0f), 2.0f));

            // Classic Phong specular via reflection vector
            Vector3D R = Vector3D::Reflect(Ldir * -1.0f, N);
            float spec = std::pow(Mathematics::Max(R.DotProduct(V), 0.0f), P.shininess);

            // Light intensity as RGB vector (assumed 0..255 per channel)
            Vector3D Li = Lgt.GetIntensity();

            // Diffuse and specular contributions
            Vector3D diff  = Li * NdotL * att;
            Vector3D specv = Li * spec  * att;

            // Modulate by material colors and accumulate
            accum.X += (diff.X  * (P.diffuseColor.R  / 255.0f)) + (specv.X * (P.specularColor.R / 255.0f));
            accum.Y += (diff.Y  * (P.diffuseColor.G  / 255.0f)) + (specv.Y * (P.specularColor.G / 255.0f));
            accum.Z += (diff.Z  * (P.diffuseColor.B  / 255.0f)) + (specv.Z * (P.specularColor.B / 255.0f));
        }

        // Clamp and pack to 8-bit RGB
        accum = accum.Constrain(0.0f, 255.0f);
        return RGBColor(uint8_t(accum.X), uint8_t(accum.Y), uint8_t(accum.Z));
    }

    /* NOTE: PhongLightShaderT is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(PhongLightShaderT)
        /* TODO: PTX_FIELD(PhongLightShaderT, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(PhongLightShaderT)
        PTX_METHOD_AUTO(PhongLightShaderT, Shade, "Shade")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(PhongLightShaderT)
        /* TODO: PTX_CTOR0(PhongLightShaderT) or PTX_CTOR(PhongLightShaderT, ...) */
    PTX_END_DESCRIBE(PhongLightShaderT)
    
};
