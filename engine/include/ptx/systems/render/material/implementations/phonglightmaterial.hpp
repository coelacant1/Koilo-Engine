#pragma once

#include <cstddef>

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/phonglightparams.hpp"
#include "../../shader/implementations/phonglightshader.hpp"

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../scene/lighting/light.hpp"

/**
 * @file phonglightmaterial.hpp
 * @brief Phong lighting material with runtime-sized light array and configurable camera/material terms.
 */
class PhongLightMaterial : public MaterialT<PhongLightParams, PhongLightShader> {
    using Base = MaterialT<PhongLightParams, PhongLightShader>;

public:
    using Base::Base;

    /**
     * @brief Construct with default light setup.
     * @param lightCount Number of lights to initialize (default 1).
     *
     * Initializes each light with a position offset along +X, white intensity, and default falloff/curve.
     */
    explicit PhongLightMaterial(std::size_t lightCount = 1) {
        this->ResizeLights(lightCount);
        for (size_t i = 0; i < lightCount; ++i) {
            // Light::Set(position, intensityRGB, falloff, curveA, curveB)
            this->lights[i].Set(
                Vector3D(1000.0f - 200.0f * float(i), 0.0f, 0.0f),
                Vector3D(255.0f, 255.0f, 255.0f),
                1000.0f, 0.5f, 0.5f
            );
        }
    }

    // --- camera control ---

    /** @brief Set camera position used for specular/eye vector calculations. */
    void SetCameraPosition(const Vector3D& p) { this->cameraPos = p; }

    // --- material terms ---

    /** @brief Set ambient color term. */
    void SetAmbient(RGBColor c)  { this->ambientColor  = c; }

    /** @brief Set diffuse color term. */
    void SetDiffuse(RGBColor c)  { this->diffuseColor  = c; }

    /** @brief Set specular color term. */
    void SetSpecular(RGBColor c) { this->specularColor = c; }

    /** @brief Set shininess exponent for specular highlight. */
    void SetShininess(float s)   { this->shininess     = s; }

    // --- light access ---

    /** @brief Mutable access to light @p i. */
    Light&       LightAt(size_t i)       { return this->lights[i]; }

    /** @brief Const access to light @p i. */
    const Light& LightAt(size_t i) const { return this->lights[i]; }

    /** @brief Number of lights stored by this material. */
    size_t LightCount() const  { return this->lights.size(); }

};
