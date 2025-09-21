#pragma once

#include "../effect.hpp"
#include "../../core/ipixelgroup.hpp"
#include "../../../../core/math/mathematics.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @brief 1D horizontal blur over pixel-group neighborhood using left/right links.
 *
 * - Blur radius scales with Effect::ratio (0..1).
 * - Writes into color buffer first, then copies back.
 */
class HorizontalBlur : public Effect {
private:
    uint8_t pixels_;  // maximum kernel diameter hint; effective radius computed from ratio

public:
    explicit HorizontalBlur(uint8_t pixels);

    void Apply(IPixelGroup* pixelGroup) override;

    PTX_BEGIN_FIELDS(HorizontalBlur)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(HorizontalBlur)
        PTX_METHOD_AUTO(HorizontalBlur, Apply, "Apply")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(HorizontalBlur)
        PTX_CTOR(HorizontalBlur, uint8_t)
    PTX_END_DESCRIBE(HorizontalBlur)

};
