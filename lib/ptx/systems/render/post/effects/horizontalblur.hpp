#pragma once

#include "../effect.hpp"
#include "../../core/ipixelgroup.hpp"
#include "../../../../core/math/mathematics.hpp"

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
};
