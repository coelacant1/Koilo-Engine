// oscilloscopeparams.hpp
#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file oscilloscopeparams.hpp
 * @brief Runtime parameter block for an audio oscilloscope material/shader.
 *
 * @details Size is stored as half-extents (width/2, height/2).
 */

struct OscilloscopeParams {
    // --- Appearance / transform ---
    Vector2D sizeHalf {80.0f, 40.0f};  ///< Half-width and half-height.
    Vector2D offset   {0.0f,  0.0f};   ///< Center position in world space.
    float    angleDeg {0.0f};          ///< Rotation around @ref offset (degrees).
    float    hueDeg   {0.0f};          ///< Hue shift applied to final gradient color (degrees).

    // --- Gradient spectrum ---
    std::vector<RGBColor> spectrum{};  ///< Runtime-sized gradient colors.

    // --- Signal ---
    const float* samples = nullptr;    ///< Non-owning pointer to bin floats.

    // --- Dynamic scaling (set by the material wrapper) ---
    float minValue = 0.0f;             ///< Minimum input value mapped to 0.
    float maxValue = 1.0f;             ///< Maximum input value mapped to 1.
    float midPoint = 0.5f;             ///< Optional midpoint reference in [0,1].

    // --- Rendering controls ---
    float lineThickness = 0.1f;        ///< Line thickness as fraction of sizeHalf.Y.
    float heightClamp   = 0.75f;       ///< Vertical amplitude clamp (0..1 of sizeHalf.Y).

    OscilloscopeParams() {
        Resize(6, 128);
    }

    OscilloscopeParams(std::size_t spectrumCount, std::size_t binCount) {
        Resize(spectrumCount, binCount);
    }

    void ResizeSpectrum(std::size_t count) {
        static const RGBColor kDefault[6] = {
            RGBColor(255,   0,   0), RGBColor(255, 255,   0),
            RGBColor(  0, 255,   0), RGBColor(  0, 255, 255),
            RGBColor(  0,   0, 255), RGBColor(255,   0, 255)
        };

        const std::size_t previous = spectrum.size();
        spectrum.resize(count);
        if (previous >= count) return;

        const std::size_t copyCount = std::min<std::size_t>(count, 6);
        for (std::size_t i = previous; i < copyCount; ++i) {
            spectrum[i] = kDefault[i];
        }
        for (std::size_t i = std::max(previous, copyCount); i < count; ++i) {
            spectrum[i] = RGBColor();
        }
    }

    void Resize(std::size_t spectrumCount, std::size_t binCount) {
        ResizeSpectrum(spectrumCount);
        binCount_ = binCount;
    }

    [[nodiscard]] std::size_t SpectrumCount() const noexcept { return spectrum.size(); }
    [[nodiscard]] std::size_t BinCount() const noexcept { return binCount_; }

    [[nodiscard]] RGBColor* SpectrumData() noexcept {
        return spectrum.empty() ? nullptr : spectrum.data();
    }

    [[nodiscard]] const RGBColor* SpectrumData() const noexcept {
        return spectrum.empty() ? nullptr : spectrum.data();
    }

    PTX_BEGIN_FIELDS(OscilloscopeParams)
        PTX_FIELD(OscilloscopeParams, sizeHalf, "Size half", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(OscilloscopeParams, offset, "Offset", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(OscilloscopeParams, angleDeg, "Angle deg", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(OscilloscopeParams, hueDeg, "Hue deg", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(OscilloscopeParams, spectrum, "Spectrum", -2147483648, 2147483647),
        PTX_FIELD(OscilloscopeParams, samples, "Samples", 0, 0),
        PTX_FIELD(OscilloscopeParams, minValue, "Min value", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(OscilloscopeParams, maxValue, "Max value", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(OscilloscopeParams, midPoint, "Mid point", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(OscilloscopeParams, lineThickness, "Line thickness", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(OscilloscopeParams, heightClamp, "Height clamp", __FLT_MIN__, __FLT_MAX__)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(OscilloscopeParams)
        PTX_METHOD_AUTO(OscilloscopeParams, ResizeSpectrum, "Resize spectrum"),
        PTX_METHOD_AUTO(OscilloscopeParams, Resize, "Resize"),
        PTX_METHOD_AUTO(OscilloscopeParams, SpectrumCount, "Spectrum count"),
        PTX_METHOD_AUTO(OscilloscopeParams, BinCount, "Bin count"),
        PTX_METHOD_OVLD0(OscilloscopeParams, SpectrumData, RGBColor *),
        PTX_METHOD_OVLD_CONST0(OscilloscopeParams, SpectrumData, const RGBColor *)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(OscilloscopeParams)
        PTX_CTOR0(OscilloscopeParams),
        PTX_CTOR(OscilloscopeParams, std::size_t, std::size_t)
    PTX_END_DESCRIBE(OscilloscopeParams)

private:
    std::size_t binCount_ = 128;
};
