#pragma once

#include "../../../../registry/reflect_macros.hpp"
// Base interfaces
#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

// Image-specific params & shader
#include "../../shader/implementations/imageparams.hpp"
#include "../../shader/implementations/imageshader.hpp"

// Concrete material = params + shader
// Explicit base alias for clarity.
using ImageMaterialBase = MaterialT<ImageParams, ImageShader>;

/**
 * @file imagematerial.hpp
 * @brief Material wrapper over ImageParams/ImageShader that exposes image, hue, and UV controls.
 */

/**
 * @class ImageMaterial
 * @brief Thin wrapper around ImageParams/ImageShader with convenience setters.
 *
 * - Construct with an Image* to bind the source image.
 * - Configure hue rotation (degrees) and UV usage via setters.
 */
class ImageMaterial : public ImageMaterialBase {
public:
    /** @brief Inherit aggregate constructor if needed: ImageMaterial{ ImageParams{...} }. */
    using ImageMaterialBase::ImageMaterialBase;

    /**
     * @brief Construct and bind an image.
     * @param img Pointer to source Image (non-owning).
     */
    explicit ImageMaterial(Image* img) {
        this->image = img;       // from ImageParams (inherited)
        this->hueAngle = 0.0f;
        this->useUV = true;
    }

    /** @brief Set hue rotation in degrees. */
    void SetHueAngle(float degrees) { this->hueAngle = degrees; }

    /** @brief Enable or disable UV sampling. */
    void UseUV(bool enabled)        { this->useUV = enabled; }

    /** @brief Replace the bound image pointer (non-owning). */
    void SetImage(Image* img)       { this->image = img; }

    PTX_BEGIN_FIELDS(ImageMaterial)
        /* TODO: PTX_FIELD(ImageMaterial, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(ImageMaterial)
        PTX_METHOD_AUTO(ImageMaterial, SetHueAngle, "Set hue angle"),
        PTX_METHOD_AUTO(ImageMaterial, UseUV, "Use uv"),
        PTX_METHOD_AUTO(ImageMaterial, SetImage, "Set image")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(ImageMaterial)
        PTX_CTOR(ImageMaterial, Image *)
    PTX_END_DESCRIBE(ImageMaterial)
    
};
