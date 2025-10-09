#pragma once

#include <cstddef>
#include <vector>

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/oscilloscopeparams.hpp"
#include "../../shader/implementations/oscilloscopeshader.hpp"

#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/signal/filter/maxfilter.hpp"
#include "../../../../core/signal/filter/minfilter.hpp"

/**
 * @file oscilloscopematerial.hpp
 * @brief Material that visualizes runtime signal samples as an oscilloscope trace with runtime-sized spectrum.
 */

/**
 * @class OscilloscopeMaterial
 * @brief Stateful material that owns smoothing filters and updates min/max/midPoint each frame.
 *
 * Exposes setters for size/position/rotation/hue/line thickness/spectrum. The input `samples`
 * pointer is external and non-owning; bind via @ref BindSamples.
 */
class OscilloscopeMaterial
    : public MaterialT<OscilloscopeParams, OscilloscopeShader> {

    using Base = MaterialT<OscilloscopeParams, OscilloscopeShader>;

public:
    /** @brief Construct with desired spectrum key count and bin count. */
    explicit OscilloscopeMaterial(std::size_t spectrumCount = 6, std::size_t binCount = 128)
        : Base(spectrumCount, binCount), maxF_(40), minF_(40) {}

    // ---------- configuration ----------

    /** @brief Set half-size (extent from center). */
    void SetSizeHalf(const Vector2D& half)  { this->sizeHalf = half; }

    /** @brief Set full size; internally converted to half-size. */
    void SetSizeFull(const Vector2D& full)  { this->sizeHalf = full / 2.0f; }

    /** @brief Set center offset. */
    void SetOffset(const Vector2D& c)       { this->offset   = c; }

    /** @brief Set rotation in degrees. */
    void SetRotationDeg(float deg)          { this->angleDeg = deg; }

    /** @brief Set base hue in degrees. */
    void SetHueDeg(float deg)               { this->hueDeg   = deg; }

    /** @brief Set line thickness as a fraction of half-height. */
    void SetLineThickness(float frac)       { this->lineThickness = frac; }

    /** @brief Clamp Y amplitude to [0..1] range multiplier. */
    void SetHeightClamp(float clamp01)      { this->heightClamp   = clamp01; }

    // Spectrum helpers

    /** @brief Set the number of spectrum keys, optionally feeding default rainbow colors. */
    void SetSpectrumCount(std::size_t count) {
        this->ResizeSpectrum(count);
    }

    /** @brief Set the number of bins (per-frame samples). */
    void SetBinCount(std::size_t count) {
        const std::size_t spectrumCount = this->SpectrumCount();
        this->Resize(spectrumCount, count);
    }

    /** @brief Current spectrum key count. */
    [[nodiscard]] std::size_t SpectrumCount() const { return this->OscilloscopeParams::SpectrumCount(); }

    /** @brief Current bin/sample count. */
    [[nodiscard]] std::size_t BinCount() const { return this->OscilloscopeParams::BinCount(); }

    /**
     * @brief Replace the spectrum from a pointer/count pair.
     */
    void SetSpectrum(const RGBColor* colors, std::size_t count) {
        if (!colors || count == 0) {
            return;
        }
        this->ResizeSpectrum(count);
        RGBColor* data = this->SpectrumData();
        if (!data) return;
        for (std::size_t i = 0; i < count; ++i) {
            data[i] = colors[i];
        }
    }

    /**
     * @brief Replace the entire spectrum from a container.
     */
    void SetSpectrum(const std::vector<RGBColor>& colors) {
        SetSpectrum(colors.data(), colors.size());
    }

    /**
     * @brief Replace the entire spectrum from a fixed-size array.
     */
    template <std::size_t Count>
    void SetSpectrum(const RGBColor (&colors)[Count]) {
        SetSpectrum(colors, Count);
    }

    /** @brief Set a single spectrum key (index clamped to [0..count-1]). */
    void SetSpectrumAt(size_t i, const RGBColor& c) {
        if (this->SpectrumCount() == 0) return;
        if (i >= this->SpectrumCount()) i = this->SpectrumCount() - 1;
        RGBColor* data = this->SpectrumData();
        if (data) data[i] = c;
    }

    /** @brief Get a single spectrum key (index clamped to [0..count-1]). */
    RGBColor GetSpectrumAt(size_t i) const {
        const std::size_t count = this->SpectrumCount();
        if (count == 0) return RGBColor();
        if (i >= count) i = count - 1;
        const RGBColor* data = this->SpectrumData();
        return data ? data[i] : RGBColor();
    }

    /** @brief Mutable pointer to the spectrum array (size determined at runtime). */
    RGBColor*       SpectrumDataMutable()       { return this->SpectrumData(); }

    /** @brief Const pointer to the spectrum array (size determined at runtime). */
    const RGBColor* SpectrumDataMutable() const { return this->SpectrumData(); }

    // ---------- data feed ----------

    /**
     * @brief Bind external pointer to floats (non-owning).
     * @param samplesPtr Pointer to an array of samples; must remain valid during use.
     */
    void BindSamples(const float* samplesPtr) { this->samples = samplesPtr; }

    /**
     * @brief Update min/max/midPoint smoothing from a representative bin.
     * @param binIndex Source sample index (default is middle bin); clamped to [0..binCount-1].
     *
     * Uses internal MinFilter/MaxFilter to smooth dynamic range and updates:
     * - @ref minValue
     * - @ref maxValue
     * - @ref midPoint  (average of smoothed min/max)
     */
    void UpdateScaling(size_t binIndex = 0) {
        if (!this->samples || this->BinCount() == 0) return;
        
        if (binIndex == 0) {
            binIndex = this->BinCount() / 2;
        }
        if (binIndex >= this->BinCount()) binIndex = this->BinCount() - 1;

        const float v = this->samples[binIndex];

        const float minFv = minF_.Filter(v);
        const float maxFv = maxF_.Filter(v);

        this->minValue = minFv;
        this->maxValue = maxFv;
        this->midPoint = (maxFv - minFv) * 0.5f + minFv;
    }

private:
    /** @brief Smoothing filters (window size 40) for dynamic range tracking. */
    MaxFilter maxF_;
    MinFilter minF_;

};
