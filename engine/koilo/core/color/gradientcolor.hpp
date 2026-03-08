#pragma once

#include <cstddef>
#include <vector>
#include "../../registry/reflect_macros.hpp"

#include "rgbcolor.hpp"
#include "../math/mathematics.hpp"

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
    std::vector<RGBColor> colors; ///< Owned color stops of the gradient (contiguous storage).
    bool isStepped = false;       ///< True to use stepped (piecewise constant) interpolation.

public:
    /**
     * @brief Construct an empty gradient (defaults to stepped = false).
     */
    GradientColor() = default;

    /**
     * @brief Construct from a pointer/count pair.
     * @param colorStops Pointer to @p count RGBColor entries (may be nullptr for empty gradient).
     * @param count Number of entries to copy from @p colorStops.
     * @param stepped Enables stepped interpolation when true.
     */
    GradientColor(const RGBColor* colorStops, std::size_t count, bool stepped = false);

    /**
     * @brief Construct from a vector of color stops.
     * @param colorStops Colors to copy into the gradient.
     * @param stepped Enables stepped interpolation when true.
     */
    explicit GradientColor(std::vector<RGBColor> colorStops, bool stepped = false);

    /**
     * @brief Calculate the color at a position along the gradient.
     * @param ratio Normalised value in [0.0, 1.0]. Values outside the range are clamped.
     * @return Interpolated (or stepped) RGB color.
     */
    [[nodiscard]] RGBColor GetColorAt(float ratio) const;

    /**
     * @brief Replace color stops from a pointer/count pair.
     * @param newColorStops Pointer to @p count RGBColor entries (may be nullptr for empty gradient).
     * @param count Number of entries to copy from @p newColorStops.
     */
    void SetColors(const RGBColor* newColorStops, std::size_t count);

    /**
     * @brief Replace color stops from a standard container.
     * @param newColorStops RGB colors to copy into the gradient.
     */
    void SetColors(const std::vector<RGBColor>& newColorStops);

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

    PTX_BEGIN_FIELDS(GradientColor)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(GradientColor)
        PTX_METHOD_AUTO(GradientColor, GetColorAt, "Get color at"),
        /* Set colors */ PTX_METHOD_OVLD(GradientColor, SetColors, void, const RGBColor *, std::size_t),
        /* Set colors */ PTX_METHOD_OVLD(GradientColor, SetColors, void, const std::vector<RGBColor> &),
        PTX_METHOD_AUTO(GradientColor, GetColorCount, "Get color count"),
        PTX_METHOD_AUTO(GradientColor, IsStepped, "Is stepped"),
        PTX_METHOD_AUTO(GradientColor, SetStepped, "Set stepped")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(GradientColor)
        PTX_CTOR0(GradientColor),
        PTX_CTOR(GradientColor, const RGBColor *, std::size_t, bool),
        PTX_CTOR(GradientColor, std::vector<RGBColor>, bool)
    PTX_END_DESCRIBE(GradientColor)

};