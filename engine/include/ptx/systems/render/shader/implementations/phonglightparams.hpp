// phonglightparams.hpp
#pragma once

#include <cstddef>
#include <array>

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../scene/lighting/light.hpp"

/**
 * @file phonglightparams.hpp
 * @brief Parameters for Phong lighting with multiple lights.
 * @tparam lightCount Number of lights compiled into the material.
 */
template <size_t lightCount>
struct PhongLightParams {
    // Material terms
    RGBColor ambientColor  {  20,  20,  20 };
    RGBColor diffuseColor  { 128, 128, 128 };
    RGBColor specularColor { 255, 255, 255 };
    float    shininess     { 32.0f };  ///< Higher value -> tighter highlight

    /// Camera (eye) position in the same space as positions/normals.
    Vector3D cameraPos { 0.0f, 0.0f, -100.0f };

    /// Lights owned by the material wrapper.
    std::array<Light, lightCount> lights{};

};
