#pragma once

#include "phongparams.hpp"
#include "../../material/materialt.hpp"
#include "../../shader/ishader.hpp"
#include "../../../../core/color/color.hpp"
#include "../../../../core/math/vector3d.hpp"

class PhongShader final : public IShader {
public:
    RGBColor Shade(const SurfaceProperties& surf,
                   const IMaterial&         m) override {

        using PhongMat = MaterialT<PhongParams, PhongShader>;
        const auto& params = m.As<PhongMat>();

        // --- Lighting Information, temporarily hardcoded
        const Vector3D lightPos     { 100.0f, 100.0f, -100.0f };
        const RGBColor lightColor   { 255, 255, 255 };
        const Vector3D viewPos      { 0.0f, 0.0f, -100.0f }; // Camera position

        Vector3D normal = Vector3D(surf.normal).UnitSphere();
        Vector3D lightDir = Vector3D(lightPos - surf.position).UnitSphere();
        Vector3D viewDir  = Vector3D(viewPos - surf.position).UnitSphere();
        Vector3D reflectDir = Vector3D::Reflect(lightDir.Multiply(-1.0f), normal);

        RGBColor ambient = params.ambientColor; // Constant base lighting

        float diffFactor = Mathematics::Max(normal.DotProduct(lightDir), 0.0f);
        RGBColor diffuse = params.diffuseColor * diffFactor; // Light scattered based on surface angle

        float specFactor = std::pow(Mathematics::Max(viewDir.DotProduct(reflectDir), 0.0f), params.shininess);
        RGBColor specular = params.specularColor * specFactor; // Sharp highlight from the light source

        return (ambient + diffuse + specular) * lightColor;
    }
};
