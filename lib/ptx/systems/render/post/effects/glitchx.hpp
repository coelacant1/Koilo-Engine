#pragma once

#include "../effect.hpp"
#include "../../core/ipixelgroup.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/platform/random.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @brief Glitch effect that shuffles pixels along X with occasional channel swaps.
 *
 * - Strength scales with Effect::ratio (0..1).
 * - Writes to color buffer first, then copies back to source.
 */
class GlitchX : public Effect {
private:
    uint8_t pixels_; // max horizontal displacement window

public:
    explicit GlitchX(uint8_t pixels);

    // Effect
    void Apply(IPixelGroup* pixelGroup) override;

    PTX_BEGIN_FIELDS(GlitchX)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(GlitchX)
        PTX_METHOD_AUTO(GlitchX, Apply, "Apply")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(GlitchX)
        PTX_CTOR(GlitchX, uint8_t)
    PTX_END_DESCRIBE(GlitchX)

};
