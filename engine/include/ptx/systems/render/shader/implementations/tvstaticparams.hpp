// tvstaticparams.hpp
#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file tvstaticparams.hpp
 * @brief Parameter block for TV static shader.
 *
 * The shader reads only from this struct; a material is expected to update fields per frame.
 * Storage for spectra is owned via std::vector to allow runtime sizing.
 */
struct TVStaticParams {
    // --- Noise (simplex) controls ---
    std::vector<RGBColor> noiseSpectrum{}; ///< Noise gradient keys (hue-shifted at runtime).
    float                 noiseGradientPeriod{0.5f}; ///< Period for gradient repetition in noise space.
    float                 noiseHueDeg{0.0f};          ///< Hue shift (degrees) applied to noiseSpectrum.
    Vector3D              noiseScale{0.5f, 0.5f, 0.5f}; ///< Scale for sampling simplex noise (xyz).
    float                 noiseZ{0.0f};              ///< Temporal/depth coordinate for animated noise.

    // --- Scanline gradient controls ---
    std::vector<RGBColor> scanSpectrum{}; ///< Scanline gradient keys.
    float                 scanGradientPeriod{100.0f}; ///< Scanline gradient period.
    float                 scanRotationDeg{90.0f};     ///< Orientation (degrees); 90° = vertical stripes.
    float                 scanShift{0.0f};            ///< Phase shift for scanline animation.

    // --- Color bars overlay (replacement for ColorTest) ---
    Vector2D barsCenter{96.0f, 72.0f}; ///< Bars region center.
    float    barsHueDeg{0.0f};         ///< Hue shift (degrees) for bars.
    Vector2D barsSize{192.0f, 96.0f};  ///< Bars region size for tiling.
    float    barsSoftness{2.0f};       ///< Edge softness in pixels.

    // --- Blend constants ---
    float scanMultiplyOpacity{0.75f}; ///< Multiply contribution opacity.

    TVStaticParams() {
        ResizeNoiseSpectrum(5);
        ResizeScanSpectrum(10);
    }

    TVStaticParams(std::size_t noiseCount, std::size_t scanCount) {
        ResizeNoiseSpectrum(noiseCount);
        ResizeScanSpectrum(scanCount);
    }

    /** @brief Resize the noise spectrum preserving existing entries when possible. */
    void ResizeNoiseSpectrum(std::size_t count) {
        static const RGBColor kDefaultNoise[5] = {
            RGBColor(255,255,255), RGBColor(0,0,0), RGBColor(255,0,0),
            RGBColor(0,255,0), RGBColor(0,0,255)
        };

        const std::size_t previous = noiseSpectrum.size();
        noiseSpectrum.resize(count);
        if (previous >= count) return;

        const std::size_t copyCount = std::min<std::size_t>(count, 5);
        for (std::size_t i = previous; i < copyCount; ++i) {
            noiseSpectrum[i] = kDefaultNoise[i];
        }
        for (std::size_t i = std::max(previous, copyCount); i < count; ++i) {
            noiseSpectrum[i] = RGBColor();
        }
    }

    /** @brief Resize the scanline spectrum preserving existing entries when possible. */
    void ResizeScanSpectrum(std::size_t count) {
        static const RGBColor kDefaultScan[10] = {
            RGBColor(0,0,0), RGBColor(0,0,0), RGBColor(255,255,255), RGBColor(0,0,0),
            RGBColor(255,255,255), RGBColor(255,255,255), RGBColor(255,255,255),
            RGBColor(0,0,0), RGBColor(0,0,0), RGBColor(255,255,255)
        };

        const std::size_t previous = scanSpectrum.size();
        scanSpectrum.resize(count);
        if (previous >= count) return;

        const std::size_t copyCount = std::min<std::size_t>(count, 10);
        for (std::size_t i = previous; i < copyCount; ++i) {
            scanSpectrum[i] = kDefaultScan[i];
        }
        for (std::size_t i = std::max(previous, copyCount); i < count; ++i) {
            scanSpectrum[i] = RGBColor();
        }
    }

    [[nodiscard]] std::size_t NoiseSpectrumCount() const noexcept { return noiseSpectrum.size(); }
    [[nodiscard]] std::size_t ScanSpectrumCount() const noexcept { return scanSpectrum.size(); }

    [[nodiscard]] RGBColor* NoiseSpectrumData() noexcept {
        return noiseSpectrum.empty() ? nullptr : noiseSpectrum.data();
    }

    [[nodiscard]] const RGBColor* NoiseSpectrumData() const noexcept {
        return noiseSpectrum.empty() ? nullptr : noiseSpectrum.data();
    }

    [[nodiscard]] RGBColor* ScanSpectrumData() noexcept {
        return scanSpectrum.empty() ? nullptr : scanSpectrum.data();
    }

    [[nodiscard]] const RGBColor* ScanSpectrumData() const noexcept {
        return scanSpectrum.empty() ? nullptr : scanSpectrum.data();
    }

    PTX_BEGIN_FIELDS(TVStaticParams)
        PTX_FIELD(TVStaticParams, noiseSpectrum, "Noise spectrum", -2147483648, 2147483647),
        PTX_FIELD(TVStaticParams, noiseGradientPeriod, "Noise gradient period", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(TVStaticParams, noiseHueDeg, "Noise hue deg", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(TVStaticParams, noiseScale, "Noise scale", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(TVStaticParams, noiseZ, "Noise Z", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(TVStaticParams, scanSpectrum, "Scan spectrum", -2147483648, 2147483647),
        PTX_FIELD(TVStaticParams, scanGradientPeriod, "Scan gradient period", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(TVStaticParams, scanRotationDeg, "Scan rotation deg", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(TVStaticParams, scanShift, "Scan shift", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(TVStaticParams, barsCenter, "Bars center", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(TVStaticParams, barsHueDeg, "Bars hue deg", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(TVStaticParams, barsSize, "Bars size", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(TVStaticParams, barsSoftness, "Bars softness", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(TVStaticParams, scanMultiplyOpacity, "Scan multiply opacity", __FLT_MIN__, __FLT_MAX__)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(TVStaticParams)
        PTX_METHOD_AUTO(TVStaticParams, ResizeNoiseSpectrum, "Resize noise spectrum"),
        PTX_METHOD_AUTO(TVStaticParams, ResizeScanSpectrum, "Resize scan spectrum"),
        PTX_METHOD_AUTO(TVStaticParams, NoiseSpectrumCount, "Noise spectrum count"),
        PTX_METHOD_AUTO(TVStaticParams, ScanSpectrumCount, "Scan spectrum count"),
        /* Noise spectrum data */ PTX_METHOD_OVLD0(TVStaticParams, NoiseSpectrumData, RGBColor *),
        /* Noise spectrum data (const) */ PTX_METHOD_OVLD_CONST0(TVStaticParams, NoiseSpectrumData, const RGBColor *),
        /* Scan spectrum data */ PTX_METHOD_OVLD0(TVStaticParams, ScanSpectrumData, RGBColor *),
        /* Scan spectrum data (const) */ PTX_METHOD_OVLD_CONST0(TVStaticParams, ScanSpectrumData, const RGBColor *)
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(TVStaticParams)
        PTX_CTOR0(TVStaticParams),
        PTX_CTOR(TVStaticParams, std::size_t, std::size_t)
    PTX_END_DESCRIBE(TVStaticParams)
};
