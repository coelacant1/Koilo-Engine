// proceduralnoiseparams.hpp
#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file proceduralnoiseparams.hpp
 * @brief Runtime parameter block for procedural noise-driven gradient shading.
 */

struct ProceduralNoiseParams {
    std::vector<RGBColor> spectrum{};           ///< Gradient spectrum (runtime-sized).

    Vector3D noiseScale       { 0.01f, 0.01f, 0.01f };  ///< Noise frequency per axis.
    float    simplexDepth     = 0.0f;                   ///< Extra Z slice (time/phase).
    float    gradientPeriod   = 1.0f;                   ///< Repeat cycle in [0..1] units.
    float    hueShiftAngleDeg = 0.0f;                   ///< Hue rotation in degrees.

    ProceduralNoiseParams() {
        ResizeSpectrum(6);
    }

    explicit ProceduralNoiseParams(std::size_t spectrumCount) {
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
        if (previous >= count) return;

        const std::size_t copyCount = std::min<std::size_t>(count, 6);
        for (std::size_t i = previous; i < copyCount; ++i) {
            spectrum[i] = kDefault[i];
        }
        for (std::size_t i = std::max(previous, copyCount); i < count; ++i) {
            spectrum[i] = RGBColor();
        }
    }

    [[nodiscard]] std::size_t SpectrumCount() const noexcept { return spectrum.size(); }

    [[nodiscard]] RGBColor* SpectrumData() noexcept {
        return spectrum.empty() ? nullptr : spectrum.data();
    }

    [[nodiscard]] const RGBColor* SpectrumData() const noexcept {
        return spectrum.empty() ? nullptr : spectrum.data();
    }

    PTX_BEGIN_FIELDS(ProceduralNoiseParams)
        PTX_FIELD(ProceduralNoiseParams, spectrum, "Spectrum", -2147483648, 2147483647),
        PTX_FIELD(ProceduralNoiseParams, noiseScale, "Noise scale", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(ProceduralNoiseParams, simplexDepth, "Simplex depth", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(ProceduralNoiseParams, gradientPeriod, "Gradient period", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(ProceduralNoiseParams, hueShiftAngleDeg, "Hue shift angle deg", __FLT_MIN__, __FLT_MAX__)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(ProceduralNoiseParams)
        PTX_METHOD_AUTO(ProceduralNoiseParams, ResizeSpectrum, "Resize spectrum"),
        PTX_METHOD_AUTO(ProceduralNoiseParams, SpectrumCount, "Spectrum count"),
        PTX_METHOD_OVLD0(ProceduralNoiseParams, SpectrumData, RGBColor *),
        PTX_METHOD_OVLD_CONST0(ProceduralNoiseParams, SpectrumData, const RGBColor *)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(ProceduralNoiseParams)
        PTX_CTOR0(ProceduralNoiseParams),
        PTX_CTOR(ProceduralNoiseParams, std::size_t)
    PTX_END_DESCRIBE(ProceduralNoiseParams)
};
