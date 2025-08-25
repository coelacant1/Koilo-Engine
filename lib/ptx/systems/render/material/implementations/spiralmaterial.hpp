#pragma once

#include <array>
#include <cstddef>

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/spiralparams.hpp"
#include "../../shader/implementations/spiralshader.hpp"

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"

/**
 * @file spiralmaterial.hpp
 * @brief Templated spiral material that blends a fixed set of N colors with adjustable width/bend.
 *
 * @tparam N Number of color keys used by the spiral shader.
 */

/**
 * @class SpiralMaterial
 * @brief Material that parameterizes a spiral effect with N key colors.
 *
 * The constructor accepts an array of N base colors and initial width/bend values.
 * Color keys may be hue-shifted relative to the original base set.
 *
 * Inherits from @c MaterialT to bind @c SpiralShaderT<N> and embed @c SpiralParams<N>.
 *
 * @tparam N Number of color keys.
 */
template <size_t N>
class SpiralMaterial : public MaterialT<SpiralParams<N>, SpiralShaderT<N>> {
    using Base = MaterialT<SpiralParams<N>, SpiralShaderT<N>>;
public:
    using Base::Base;

    /**
     * @brief Construct with base colors and initial width/bend.
     * @param rgbColors Pointer to an array of N RGBColor entries (not owned).
     * @param width     Initial spiral width parameter.
     * @param bend      Initial spiral bend parameter.
     */
    SpiralMaterial(const RGBColor* rgbColors, float width, float bend) {
        this->width = width;
        this->bend  = bend;
        this->baseColors = rgbColors;
        for (size_t i = 0; i < N; ++i) this->colors[i] = rgbColors[i];
    }

    // --- API parity with the old class ---

    /** @brief Set position offset. */
    void SetPositionOffset(Vector2D v)   { this->positionOffset = v; }

    /** @brief Set rotation offset (vector form). */
    void SetRotationOffset(Vector2D v)   { this->rotationOffset = v; }

    /** @brief Set rotation angle in degrees. */
    void SetRotationAngle(float deg)     { this->rotationAngle  = deg; }

    /** @brief Set spiral width parameter. */
    void SetWidth(float w)               { this->width          = w; }

    /** @brief Set spiral bend parameter. */
    void SetBend(float b)                { this->bend           = b; }

    /**
     * @brief Apply a hue shift to the working color keys.
     * @param hueDeg Hue shift in degrees applied to each base color.
     * @note Requires @c baseColors to be non-null; otherwise no change is made.
     */
    void HueShift(float hueDeg) {
        if (!this->baseColors) return;
        for (size_t i = 0; i < N; ++i)
            this->colors[i] = this->baseColors[i].HueShift(hueDeg);
    }

    /**
     * @brief Restore working colors from the original base color set.
     * @note Requires @c baseColors to be non-null; otherwise no change is made.
     */
    void UpdateRGB() {
        if (!this->baseColors) return;
        for (size_t i = 0; i < N; ++i)
            this->colors[i] = this->baseColors[i];
    }
};
