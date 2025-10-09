#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "../../../../registry/reflect_macros.hpp"

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"

/**
 * @file audioreactiveparams.hpp
 * @brief Runtime-sized parameter block for audio-reactive gradient materials.
 */

struct AudioReactiveParams {
    // Transform / appearance
    Vector2D sizeHalf {96.0f, 48.0f};   ///< Half width/height (extent from center).
    Vector2D offset   {0.0f,  0.0f};    ///< Center position.
    float    angleDeg {0.0f};           ///< Rotation in degrees about @ref offset.
    float    hueDeg   {0.0f};           ///< Hue shift in degrees.

    // Circular ring mode
    bool     circular {false};          ///< Enable circular (ring) rendering.
    float    radius   {75.0f};          ///< Radius used when @ref circular is true.

    // Behavior flags
    bool     bounce   {false};          ///< Enable per-bin bounce processing.

    // Spectrum data (owned externally)
    const float* samples {nullptr};     ///< Pointer to spectrum values (0..1 suggested).

    // Optional bounced data (written by material Update)
    std::vector<float> bounceData{};    ///< Per-bin output when @ref bounce is true.

    // Gradient keys (unused entries remain zero if > canonical palette)
    std::vector<RGBColor> spectrum{};   ///< Runtime-sized gradient color keys.

    AudioReactiveParams() {
        ResizeSpectrum(6);
        ResizeSamples(128);
    }

    AudioReactiveParams(std::size_t spectrumCount, std::size_t sampleCount) {
        ResizeSpectrum(spectrumCount);
        ResizeSamples(sampleCount);
    }

    /**
     * @brief Resize spectrum storage while seeding canonical colors for new slots.
     */
    void ResizeSpectrum(std::size_t count) {
        static const RGBColor kDefault[6] = {
            RGBColor(255, 0,   0), RGBColor(255, 255,   0),
            RGBColor(  0, 255, 0), RGBColor(  0, 255, 255),
            RGBColor(  0, 0, 255), RGBColor(255,   0, 255)
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

    /**
     * @brief Resize bounce buffer storage to match sample count.
     */
    void ResizeSamples(std::size_t count) {
        sampleCount = count;
        bounceData.assign(count, 0.0f);
    }

    /** @brief Number of gradient keys currently stored. */
    [[nodiscard]] std::size_t SpectrumCount() const noexcept { return spectrum.size(); }

    /** @brief Number of spectrum samples expected. */
    [[nodiscard]] std::size_t SampleCount() const noexcept { return sampleCount; }

    PTX_BEGIN_FIELDS(AudioReactiveParams)
        PTX_FIELD(AudioReactiveParams, sizeHalf, "Size half", 0, 0),
        PTX_FIELD(AudioReactiveParams, offset, "Offset", 0, 0),
        PTX_FIELD(AudioReactiveParams, angleDeg, "Angle deg", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(AudioReactiveParams, hueDeg, "Hue deg", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(AudioReactiveParams, circular, "Circular", 0, 1),
        PTX_FIELD(AudioReactiveParams, radius, "Radius", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(AudioReactiveParams, bounce, "Bounce", 0, 1),
        PTX_FIELD(AudioReactiveParams, samples, "Samples", 0, 0),
        PTX_FIELD(AudioReactiveParams, bounceData, "Bounce data", -2147483648, 2147483647),
        PTX_FIELD(AudioReactiveParams, spectrum, "Spectrum", -2147483648, 2147483647)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(AudioReactiveParams)
        PTX_METHOD_AUTO(AudioReactiveParams, ResizeSpectrum, "Resize spectrum"),
        PTX_METHOD_AUTO(AudioReactiveParams, ResizeSamples, "Resize samples"),
        PTX_METHOD_AUTO(AudioReactiveParams, SpectrumCount, "Spectrum count"),
        PTX_METHOD_AUTO(AudioReactiveParams, SampleCount, "Sample count")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(AudioReactiveParams)
        PTX_CTOR0(AudioReactiveParams),
        PTX_CTOR(AudioReactiveParams, std::size_t, std::size_t)
    PTX_END_DESCRIBE(AudioReactiveParams)

private:
    std::size_t sampleCount{0};
};
