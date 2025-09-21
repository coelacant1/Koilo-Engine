#pragma once

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"
#include "../../../../registry/reflect_macros.hpp"

#include "../../shader/implementations/uvmapparams.hpp"
#include "../../shader/implementations/uvmapshader.hpp"

/**
 * @file uvmapmaterial.hpp
 * @brief UV-mapped texture material with legacy ergonomics (ctor(Image*), SetHueAngle, SetFlip*, SetSize).
 */

/**
 * @class UVMapMaterial
 * @brief Textures a surface using UVs, parameterized by @ref UVMapParams and shaded by @ref UVMapShader.
 *
 * Provides simple setters for common controls (hue angle, U/V flips, size) and an Image-bound
 * constructor to mirror prior usage patterns.
 */
class UVMapMaterial : public MaterialT<UVMapParams, UVMapShader> {
    using Base = MaterialT<UVMapParams, UVMapShader>;
public:
    using Base::Base; ///< Inherit aggregate/forwarding constructor if desired

    /**
     * @brief Construct and bind an image with default mapping controls.
     * @param img Pointer to the source @c Image (non-owning).
     *
     * Defaults:
     * - size   = (1,1)
     * - flipU  = true  (u' = 1 - u)
     * - flipV  = false
     * - hueAngle = 0
     */
    explicit UVMapMaterial(Image* img) {
        this->image = img;
        this->size  = {1.0f, 1.0f};
        this->flipU = true;
        this->flipV = false;
        this->hueAngle = 0.0f;
    }

    /** @brief Set hue rotation in degrees. */
    void SetHueAngle(float degrees) { this->hueAngle = degrees; }

    /** @brief Flip U coordinate when true. */
    void SetFlipU(bool on)          { this->flipU = on; }

    /** @brief Flip V coordinate when true. */
    void SetFlipV(bool on)          { this->flipV = on; }

    /** @brief Set mapping size scale. */
    void SetSize(Vector2D s)        { this->size = s; }

    /** @brief Rebind the source image (non-owning). */
    void SetImage(Image* img)       { this->image = img; }

    PTX_BEGIN_FIELDS(UVMapMaterial)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(UVMapMaterial)
        PTX_METHOD_AUTO(UVMapMaterial, SetHueAngle, "Set hue angle"),
        PTX_METHOD_AUTO(UVMapMaterial, SetFlipU, "Set flip u"),
        PTX_METHOD_AUTO(UVMapMaterial, SetFlipV, "Set flip v"),
        PTX_METHOD_AUTO(UVMapMaterial, SetSize, "Set size"),
        PTX_METHOD_AUTO(UVMapMaterial, SetImage, "Set image")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(UVMapMaterial)
        PTX_CTOR(UVMapMaterial, Image *)
    PTX_END_DESCRIBE(UVMapMaterial)

};
