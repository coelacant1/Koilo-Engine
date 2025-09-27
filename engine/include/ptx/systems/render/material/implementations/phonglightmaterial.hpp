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
 * @brief Phong lighting material with a fixed-size light array and configurable camera/material terms.
 *
 * @tparam lightCount Number of lights stored in the parameter block.
 */
template <size_t lightCount>
class PhongLightMaterial : public MaterialT<PhongLightParams<lightCount>, PhongLightShaderT<lightCount>> {
    using Base = MaterialT<PhongLightParams<lightCount>, PhongLightShaderT<lightCount>>;

public:
    using Base::Base;

    /**
     * @brief Construct with default light setup.
     *
     * Initializes each light with a position offset along +X, white intensity, and default falloff/curve.
     */
    PhongLightMaterial() {
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
    constexpr size_t LightCount() const  { return lightCount; }

};
