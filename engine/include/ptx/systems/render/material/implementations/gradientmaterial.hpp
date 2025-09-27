#pragma once

#include <array>
#include <cstddef>  // for size_t

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/gradientparams.hpp"
#include "../../shader/implementations/gradientshader.hpp"

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"

/**
 * @file gradientmaterial.hpp
 * @brief MaterialT<GradientParams<N>, GradientShaderT<N>> wrapper for N-stop gradients.
 *
 * @tparam N Number of colors in the gradient.
 */
template <size_t N>
class GradientMaterial : public MaterialT<GradientParams<N>, GradientShaderT<N>> {
    using Base = MaterialT<GradientParams<N>, GradientShaderT<N>>;

public:
    using Base::Base; ///< Allow aggregate initialization when desired.

    /**
     * @brief Construct with external base color array and initial configuration.
     * @param rgbColors       Pointer to an array of N base colors (non-owning).
     * @param gradientPeriod  Repetition period for the gradient.
     * @param isRadial        True for radial gradient, false for linear.
     * @param isStepped       True to render discrete steps instead of smooth blend.
     * @note The pointer @p rgbColors is stored in params as @c baseColors (non-owning).
     */
    GradientMaterial(RGBColor* rgbColors,
                     float      gradientPeriod,
                     bool       isRadial,
                     bool       isStepped = false)
    {
        this->gradientPeriod = gradientPeriod;
        this->isRadial       = isRadial;
        this->isStepped      = isStepped;
        this->baseColors     = rgbColors;
        UpdateGradient(rgbColors);
    }

    /**
     * @brief Copy N colors from an external buffer into the active gradient.
     * @param rgbColors Pointer to an array of N colors.
     */
    void UpdateGradient(RGBColor* rgbColors) {
        for (size_t i = 0; i < N; ++i) {
            this->colors[i] = rgbColors[i];
        }
    }

    /** @brief Set translation offset in 2D. */
    void SetPositionOffset(Vector2D positionOffset) { this->positionOffset = positionOffset; }

    /** @brief Set rotation origin offset in 2D. */
    void SetRotationOffset(Vector2D rotationOffset) { this->rotationOffset = rotationOffset; }

    /** @brief Set rotation angle (degrees). */
    void SetRotationAngle(float rotationAngle)      { this->rotationAngle  = rotationAngle;  }

    /** @brief Set gradient repetition period. */
    void SetGradientPeriod(float gradientPeriod)    { this->gradientPeriod = gradientPeriod; }

    /** @brief Shift gradient phase by @p ratio (0..1). */
    void GradientShift(float ratio)                 { this->gradientShift  = ratio;          }

    /**
     * @brief Apply hue shift to active colors based on @ref baseColors.
     * @param hueDeg Hue delta in degrees.
     * @note No-op if @ref baseColors is null.
     */
    void HueShift(float hueDeg) {
        if (!this->baseColors) return;
        for (size_t i = 0; i < N; ++i) {
            this->colors[i] = this->baseColors[i].HueShift(hueDeg);
        }
    }

    /**
     * @brief Refresh active colors from @ref baseColors verbatim.
     * @note No-op if @ref baseColors is null.
     */
    void UpdateRGB() {
        if (!this->baseColors) return;
        for (size_t i = 0; i < N; ++i) {
            this->colors[i] = this->baseColors[i];
        }
    }

};
