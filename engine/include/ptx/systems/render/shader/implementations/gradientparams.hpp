#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>
#include "../../../../registry/reflect_macros.hpp"

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"

/**
 * @file gradientparams.hpp
 * @brief Runtime-sized parameter block for linear/radial gradients.
 *
 * Stores dynamic color keys alongside spatial and rendering controls. Color storage is
 * owned internally via @ref std::vector while @ref baseColors remains a non-owning pointer
 * for features such as hue shifting relative to an external palette.
 */

struct GradientParams {
    /** Owned color keys used by the gradient. */
    std::vector<RGBColor> colors{};

    /** Optional non-owning palette pointer for features like hue shifting. */
    const RGBColor*        baseColors{nullptr};

    /** XY translation applied to the gradient. */
    Vector2D positionOffset{};

    /** Pivot (XY) about which @ref rotationAngle is applied. */
    Vector2D rotationOffset{};

    /** Period for repeating the gradient pattern. */
    float gradientPeriod{1.0f};

    /** Rotation angle in degrees (around @ref rotationOffset). */
    float rotationAngle{0.0f};

    /** Normalized shift (0..1) applied along the gradient axis. */
    float gradientShift{0.0f};

    /** Render as radial (true) or linear (false) gradient. */
    bool isRadial{false};

    /** Render with discrete steps (true) or smoothly interpolated (false). */
    bool isStepped{false};

    GradientParams() = default;

    /**
     * @brief Construct with a desired number of color keys.
     * @param colorCount Number of entries to allocate in @ref colors.
     *
     * Initializes the first six entries with a canonical rainbow sequence to preserve
     * historical defaults; additional slots (if any) remain black.
     */
    explicit GradientParams(std::size_t colorCount) {
        Resize(colorCount);
    }

    /**
     * @brief Resize color storage while preserving existing entries.
     * @param colorCount New number of gradient keys.
     */
    void Resize(std::size_t colorCount) {
        const RGBColor kDefault[6] = {
            RGBColor(255,   0,   0), RGBColor(255, 255,   0),
            RGBColor(  0, 255,   0), RGBColor(  0, 255, 255),
            RGBColor(  0,   0, 255), RGBColor(255,   0, 255)
        };

        auto previousSize = colors.size();
        colors.resize(colorCount);
        if (previousSize >= colorCount) return;

        const std::size_t copyCount = (colorCount < 6) ? colorCount : 6;
        for (std::size_t i = previousSize; i < copyCount; ++i) {
            colors[i] = kDefault[i];
        }
        for (std::size_t i = std::max(previousSize, copyCount); i < colorCount; ++i) {
            colors[i] = RGBColor();
        }
    }

    /** @brief Convenience accessor for the number of active gradient keys. */
    [[nodiscard]] std::size_t ColorCount() const noexcept { return colors.size(); }

    PTX_BEGIN_FIELDS(GradientParams)
        PTX_FIELD(GradientParams, colors, "Colors", -2147483648, 2147483647),
        PTX_FIELD(GradientParams, baseColors, "Base colors", 0, 0),
        PTX_FIELD(GradientParams, positionOffset, "Position offset", 0, 0),
        PTX_FIELD(GradientParams, rotationOffset, "Rotation offset", 0, 0),
        PTX_FIELD(GradientParams, gradientPeriod, "Gradient period", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(GradientParams, rotationAngle, "Rotation angle", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(GradientParams, gradientShift, "Gradient shift", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(GradientParams, isRadial, "Is radial", 0, 1),
        PTX_FIELD(GradientParams, isStepped, "Is stepped", 0, 1)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(GradientParams)
        PTX_METHOD_AUTO(GradientParams, Resize, "Resize"),
        PTX_METHOD_AUTO(GradientParams, ColorCount, "Color count")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(GradientParams)
        PTX_CTOR0(GradientParams),
        PTX_CTOR(GradientParams, std::size_t)
    PTX_END_DESCRIBE(GradientParams)

};
