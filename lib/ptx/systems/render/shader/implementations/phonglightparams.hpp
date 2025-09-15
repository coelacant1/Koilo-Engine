// phonglightparams.hpp
#pragma once

#include <cstddef>
#include <array>
#include "../../../../registry/reflect_macros.hpp"

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

    /* NOTE: PhongLightParams is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(PhongLightParams)
        PTX_FIELD(PhongLightParams, ambientColor, "Ambient color", 0, 0),
        PTX_FIELD(PhongLightParams, diffuseColor, "Diffuse color", 0, 0),
        PTX_FIELD(PhongLightParams, specularColor, "Specular color", 0, 0),
        PTX_FIELD(PhongLightParams, shininess, "Shininess", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(PhongLightParams, cameraPos, "Camera pos", 0, 0),
        PTX_FIELD(PhongLightParams, lights, "Lights", 0, 0)
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(PhongLightParams)
        /* TODO: PTX_METHOD_AUTO(PhongLightParams, Method, "Doc") */
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(PhongLightParams)
        /* TODO: PTX_CTOR0(PhongLightParams) or PTX_CTOR(PhongLightParams, ...) */
    PTX_END_DESCRIBE(PhongLightParams)
    
};
