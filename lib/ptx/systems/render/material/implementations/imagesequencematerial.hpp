#pragma once

#include "../../../../registry/reflect_macros.hpp"
// Base interfaces
#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

// Sequence-specific params & shader
#include "../../shader/implementations/imagesequenceparams.hpp"
#include "../../shader/implementations/imagesequenceshader.hpp"

// Concrete base (template pairing)
using ImageSequenceMaterialBase = MaterialT<ImageSequenceParams, ImageSequenceShader>;

/**
 * @file imagesequencematerial.hpp
 * @brief Material wrapper over ImageSequenceParams/ImageSequenceShader with sequence, hue, and UV controls.
 */

/**
 * @class ImageSequenceMaterial
 * @brief Wrapper material that binds an ImageSequence and provides simple update and configuration hooks.
 */
class ImageSequenceMaterial : public ImageSequenceMaterialBase {
public:
    using ImageSequenceMaterialBase::ImageSequenceMaterialBase;

    /**
     * @brief Construct and bind an image sequence.
     * @param seq Pointer to ImageSequence (non-owning).
     */
    explicit ImageSequenceMaterial(ImageSequence* seq) {
        this->sequence = seq;   // from ImageSequenceParams (inherited)
        this->hueAngle = 0.0f;
        this->useUV    = true;
    }

    /** @brief Set hue rotation in degrees. */
    void SetHueAngle(float degrees) { this->hueAngle = degrees; }

    /** @brief Enable or disable UV sampling. */
    void UseUV(bool enabled)        { this->useUV = enabled;    }

    /** @brief Replace the bound sequence pointer (non-owning). */
    void SetSequence(ImageSequence* seq) { this->sequence = seq; }

    /**
     * @brief Per-frame update hook.
     * @param dt Time delta in seconds (unused here).
     *
     * Calls the bound sequence’s Update() to advance frames.
     */
    void Update(float /*dt*/) override {
        if (this->sequence) this->sequence->Update();
    }

    PTX_BEGIN_FIELDS(ImageSequenceMaterial)
        /* TODO: PTX_FIELD(ImageSequenceMaterial, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(ImageSequenceMaterial)
        PTX_METHOD_AUTO(ImageSequenceMaterial, SetHueAngle, "Set hue angle"),
        PTX_METHOD_AUTO(ImageSequenceMaterial, UseUV, "Use uv"),
        PTX_METHOD_AUTO(ImageSequenceMaterial, SetSequence, "Set sequence"),
        PTX_METHOD_AUTO(ImageSequenceMaterial, Update, "Update")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(ImageSequenceMaterial)
        PTX_CTOR(ImageSequenceMaterial, ImageSequence *)
    PTX_END_DESCRIBE(ImageSequenceMaterial)
    
};
