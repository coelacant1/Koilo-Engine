// phonglightparams.hpp
#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../scene/lighting/light.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file phonglightparams.hpp
 * @brief Runtime parameters for Phong lighting with multiple lights.
 */

struct PhongLightParams {
    // Material terms
    RGBColor ambientColor  {  20,  20,  20 };
    RGBColor diffuseColor  { 128, 128, 128 };
    RGBColor specularColor { 255, 255, 255 };
    float    shininess     { 32.0f };  ///< Higher value -> tighter highlight

    /// Camera (eye) position in the same space as positions/normals.
    Vector3D cameraPos { 0.0f, 0.0f, -100.0f };

    /// Lights owned by the material wrapper.
    std::vector<Light> lights{};

    PhongLightParams() {
        ResizeLights(1);
    }

    explicit PhongLightParams(std::size_t lightCount) {
        ResizeLights(lightCount);
    }

    void ResizeLights(std::size_t count) {
        lights.resize(count);
        // Initialize with default light settings
        for (auto& light : lights) {
            // Use Light::Set(position, intensity, falloff, curveA, curveB)
            light.Set(
                Vector3D(0.0f, 0.0f, -50.0f),     // position
                Vector3D(255.0f, 255.0f, 255.0f), // intensity
                100.0f,                            // falloff
                0.01f,                             // curveA
                0.001f                             // curveB
            );
        }
    }

    [[nodiscard]] std::size_t LightCount() const noexcept { return lights.size(); }

    [[nodiscard]] Light* LightData() noexcept {
        return lights.empty() ? nullptr : lights.data();
    }

    [[nodiscard]] const Light* LightData() const noexcept {
        return lights.empty() ? nullptr : lights.data();
    }

    PTX_BEGIN_FIELDS(PhongLightParams)
        PTX_FIELD(PhongLightParams, ambientColor, "Ambient color", 0, 255),
        PTX_FIELD(PhongLightParams, diffuseColor, "Diffuse color", 0, 255),
        PTX_FIELD(PhongLightParams, specularColor, "Specular color", 0, 255),
        PTX_FIELD(PhongLightParams, shininess, "Shininess", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(PhongLightParams, cameraPos, "Camera pos", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(PhongLightParams, lights, "Lights", -2147483648, 2147483647)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(PhongLightParams)
        PTX_METHOD_AUTO(PhongLightParams, ResizeLights, "Resize lights"),
        PTX_METHOD_AUTO(PhongLightParams, LightCount, "Light count"),
        PTX_METHOD_OVLD0(PhongLightParams, LightData, Light *),
        PTX_METHOD_OVLD_CONST0(PhongLightParams, LightData, const Light *)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(PhongLightParams)
        PTX_CTOR0(PhongLightParams),
        PTX_CTOR(PhongLightParams, std::size_t)
    PTX_END_DESCRIBE(PhongLightParams)
};
