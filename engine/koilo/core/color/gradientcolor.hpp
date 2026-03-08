// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <vector>
#include <koilo/registry/reflect_macros.hpp>

#include "color888.hpp"
#include "colorpalette.hpp"
#include <koilo/core/math/mathematics.hpp>


namespace koilo {

/**
 * @file GradientColor.hpp
 * @brief Runtime-sized color gradient helper supporting smooth and stepped interpolation.
 * @date 29/06/2025
 * @version 2.0
 */

/**
 * @class GradientColor
 * @brief Represents a runtime-sized color gradient with optional stepped transitions.
 *
 * Callers may configure the gradient via pointer/count pairs or standard containers.
 * Memory ownership is internal; repeated updates reuse the same storage to minimise
 * heap churn on constrained targets.
 */
class GradientColor {
private:
    std::vector<Color888> colors; ///< Owned color stops of the gradient (contiguous storage).
    bool isStepped = false;       ///< True to use stepped (piecewise constant) interpolation.

public:
    /**
     * @brief Construct an empty gradient (defaults to stepped = false).
     */
    GradientColor() = default;

    /**
     * @brief Construct from a pointer/count pair.
     * @param colorStops Pointer to @p count Color888 entries (may be nullptr for empty gradient).
     * @param count Number of entries to copy from @p colorStops.
     * @param stepped Enables stepped interpolation when true.
     */
    GradientColor(const Color888* colorStops, std::size_t count, bool stepped = false);

    /**
     * @brief Construct from a vector of color stops.
     * @param colorStops Colors to copy into the gradient.
     * @param stepped Enables stepped interpolation when true.
     */
    explicit GradientColor(std::vector<Color888> colorStops, bool stepped = false);

    /**
     * @brief Calculate the color at a position along the gradient.
     * @param ratio Normalised value in [0.0, 1.0]. Values outside the range are clamped.
     * @return Interpolated (or stepped) RGB color.
     */
    [[nodiscard]] Color888 GetColorAt(float ratio) const;

    /**
     * @brief Replace color stops from a pointer/count pair.
     * @param newColorStops Pointer to @p count Color888 entries (may be nullptr for empty gradient).
     * @param count Number of entries to copy from @p newColorStops.
     */
    void SetColors(const Color888* newColorStops, std::size_t count);

    /**
     * @brief Replace color stops from a standard container.
     * @param newColorStops RGB colors to copy into the gradient.
     */
    void SetColors(const std::vector<Color888>& newColorStops);

    // Script-friendly: set colors from a ColorPalette.
    void SetColorsFromPalette(const ColorPalette& palette) {
        SetColors(palette.Data(), palette.Size());
    }

    /**
     * @brief Retrieve the number of color stops currently stored.
     */
    [[nodiscard]] std::size_t GetColorCount() const noexcept;

    /**
     * @brief Query whether stepped interpolation is active.
     */
    [[nodiscard]] bool IsStepped() const noexcept;

    /**
     * @brief Toggle stepped interpolation.
     */
    void SetStepped(bool stepped) noexcept;

    KL_BEGIN_FIELDS(GradientColor)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(GradientColor)
        KL_METHOD_AUTO(GradientColor, GetColorAt, "Get color at"),
        /* Set colors */ KL_METHOD_OVLD(GradientColor, SetColors, void, const Color888 *, std::size_t),
        /* Set colors */ KL_METHOD_OVLD(GradientColor, SetColors, void, const std::vector<Color888> &),
        KL_METHOD_AUTO(GradientColor, GetColorCount, "Get color count"),
        KL_METHOD_AUTO(GradientColor, IsStepped, "Is stepped"),
        KL_METHOD_AUTO(GradientColor, SetStepped, "Set stepped"),
        KL_METHOD_AUTO(GradientColor, SetColorsFromPalette, "Set colors from palette")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(GradientColor)
        KL_CTOR0(GradientColor),
        KL_CTOR(GradientColor, const Color888 *, std::size_t, bool),
        KL_CTOR(GradientColor, std::vector<Color888>, bool)
    KL_END_DESCRIBE(GradientColor)

};

} // namespace koilo
