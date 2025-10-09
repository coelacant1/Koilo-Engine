#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "../../../../registry/reflect_macros.hpp"

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"

/**
 * @file horizontalrainbowparams.hpp
 * @brief Runtime parameter block for a horizontal rainbow gradient with dynamic spectrum keys.
 *
 * Defines color spectrum keys, position offset for scrolling, rotation (in degrees),
 * periodicity of the gradient in world units, and per-update scroll speed.
 */

/**
 * @struct HorizontalRainbowParams
 * @brief Parameters used by a horizontal rainbow gradient shader/material.
 *
 * Defaults:
 * - spectrum: classic 6-color rainbow (extra entries, if any, remain {0,0,0})
 * - rotationDeg: 90° -> horizontal bands
 * - gradientPeriod: repeat distance in world units
 * - scrollSpeed: delta applied to the X axis each Update()
 */
struct HorizontalRainbowParams {
    /// Color keys for the gradient.
    std::vector<RGBColor> spectrum{};

    Vector2D positionOffset {0.0f, 0.0f}; ///< Animated scroll offset (world units).
    float    rotationDeg    = 90.0f;      ///< Rotation in degrees (90° -> horizontal gradient).
    float    gradientPeriod = 96.0f;      ///< Repeat distance in world units.
    float    scrollSpeed    = -1.0f;      ///< Units per Update() applied along +X.

    HorizontalRainbowParams() {
        ResizeSpectrum(6);
    }

    explicit HorizontalRainbowParams(std::size_t spectrumCount) {
        ResizeSpectrum(spectrumCount);
    }

    void ResizeSpectrum(std::size_t count) {
        static const RGBColor kDefault[6] = {
            RGBColor(255,   0,   0), RGBColor(255, 255,   0),
            RGBColor(  0, 255,   0), RGBColor(  0, 255, 255),
            RGBColor(  0,   0, 255), RGBColor(255,   0, 255)
        };

        const std::size_t previous = spectrum.size();
        spectrum.resize(count);
        const std::size_t copyCount = std::min<std::size_t>(count, 6);
        for (std::size_t i = previous; i < copyCount; ++i) {
            spectrum[i] = kDefault[i];
        }
        for (std::size_t i = std::max(previous, copyCount); i < count; ++i) {
            spectrum[i] = RGBColor();
        }
    }

    [[nodiscard]] std::size_t SpectrumCount() const noexcept { return spectrum.size(); }

    PTX_BEGIN_FIELDS(HorizontalRainbowParams)
        PTX_FIELD(HorizontalRainbowParams, spectrum, "Spectrum", -2147483648, 2147483647),
        PTX_FIELD(HorizontalRainbowParams, positionOffset, "Position offset", 0, 0),
        PTX_FIELD(HorizontalRainbowParams, rotationDeg, "Rotation deg", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(HorizontalRainbowParams, gradientPeriod, "Gradient period", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(HorizontalRainbowParams, scrollSpeed, "Scroll speed", __FLT_MIN__, __FLT_MAX__)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(HorizontalRainbowParams)
        PTX_METHOD_AUTO(HorizontalRainbowParams, ResizeSpectrum, "Resize spectrum"),
        PTX_METHOD_AUTO(HorizontalRainbowParams, SpectrumCount, "Spectrum count")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(HorizontalRainbowParams)
        PTX_CTOR0(HorizontalRainbowParams),
        PTX_CTOR(HorizontalRainbowParams, std::size_t)
    PTX_END_DESCRIBE(HorizontalRainbowParams)

};
