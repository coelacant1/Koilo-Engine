// spectrumanalyzerparams.hpp
#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file spectrumanalyzerparams.hpp
 * @brief Runtime parameter block for the spectrum analyzer material/shader.
 */

struct SpectrumAnalyzerParams {
    // Transform / appearance
    Vector2D sizeHalf {96.0f, 48.0f};    ///< Half width/height (extents from center).
    Vector2D offset   {0.0f,  0.0f};     ///< Center position.
    float    angleDeg {0.0f};            ///< Rotation about @ref offset in degrees.
    float    hueDeg   {0.0f};            ///< Hue shift applied to the gradient (degrees).

    // Y-shaping & options
    bool  mirrorY     {false};        ///< Mirror around X axis.
    bool  flipY       {false};        ///< Invert Y ramp.
    bool  bounce      {false};        ///< Use BouncePhysics per bin.
    float heightScale {3.0f};         ///< Additional gain applied to bar height.

    // Data
    const float*       samples   {nullptr}; ///< External pointer to @ref binCount floats (non-owning).
    std::vector<float> bounceData{};        ///< Smoothed or mirrored sample data owned by the params.

    // Gradient keys (defaults to a 6-key rainbow; additional entries are zeroed)
    std::vector<RGBColor> spectrum{};

    SpectrumAnalyzerParams() {
        Resize(6, 128);
    }

    SpectrumAnalyzerParams(std::size_t spectrumCount, std::size_t binCount) {
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

    void ResizeBins(std::size_t count) {
        bounceData.resize(count);
        if (count > 0) {
            std::fill(bounceData.begin(), bounceData.end(), 0.0f);
        }
    }

    void Resize(std::size_t spectrumCount, std::size_t binCount) {
        ResizeSpectrum(spectrumCount);
        ResizeBins(binCount);
    }

    [[nodiscard]] std::size_t SpectrumCount() const noexcept { return spectrum.size(); }
    [[nodiscard]] std::size_t BinCount() const noexcept { return bounceData.size(); }

    [[nodiscard]] RGBColor* SpectrumData() noexcept {
        return spectrum.empty() ? nullptr : spectrum.data();
    }

    [[nodiscard]] const RGBColor* SpectrumData() const noexcept {
        return spectrum.empty() ? nullptr : spectrum.data();
    }

    [[nodiscard]] float* BounceData() noexcept {
        return bounceData.empty() ? nullptr : bounceData.data();
    }

    [[nodiscard]] const float* BounceData() const noexcept {
        return bounceData.empty() ? nullptr : bounceData.data();
    }

    PTX_BEGIN_FIELDS(SpectrumAnalyzerParams)
        PTX_FIELD(SpectrumAnalyzerParams, sizeHalf, "Size half", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(SpectrumAnalyzerParams, offset, "Offset", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(SpectrumAnalyzerParams, angleDeg, "Angle deg", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(SpectrumAnalyzerParams, hueDeg, "Hue deg", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(SpectrumAnalyzerParams, mirrorY, "Mirror Y", 0, 1),
        PTX_FIELD(SpectrumAnalyzerParams, flipY, "Flip Y", 0, 1),
        PTX_FIELD(SpectrumAnalyzerParams, bounce, "Bounce", 0, 1),
        PTX_FIELD(SpectrumAnalyzerParams, heightScale, "Height scale", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(SpectrumAnalyzerParams, samples, "Samples", 0, 0),
        PTX_FIELD(SpectrumAnalyzerParams, bounceData, "Bounce data", -2147483648, 2147483647),
        PTX_FIELD(SpectrumAnalyzerParams, spectrum, "Spectrum", -2147483648, 2147483647)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(SpectrumAnalyzerParams)
        PTX_METHOD_AUTO(SpectrumAnalyzerParams, ResizeSpectrum, "Resize spectrum"),
        PTX_METHOD_AUTO(SpectrumAnalyzerParams, ResizeBins, "Resize bins"),
        PTX_METHOD_AUTO(SpectrumAnalyzerParams, Resize, "Resize"),
        PTX_METHOD_AUTO(SpectrumAnalyzerParams, SpectrumCount, "Spectrum count"),
        PTX_METHOD_AUTO(SpectrumAnalyzerParams, BinCount, "Bin count"),
        /* Spectrum data */ PTX_METHOD_OVLD0(SpectrumAnalyzerParams, SpectrumData, RGBColor *),
        /* Spectrum data (const) */ PTX_METHOD_OVLD_CONST0(SpectrumAnalyzerParams, SpectrumData, const RGBColor *),
        /* Bounce data */ PTX_METHOD_OVLD0(SpectrumAnalyzerParams, BounceData, float *),
        /* Bounce data (const) */ PTX_METHOD_OVLD_CONST0(SpectrumAnalyzerParams, BounceData, const float *)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(SpectrumAnalyzerParams)
        PTX_CTOR0(SpectrumAnalyzerParams),
        PTX_CTOR(SpectrumAnalyzerParams, std::size_t, std::size_t)
    PTX_END_DESCRIBE(SpectrumAnalyzerParams)
};
