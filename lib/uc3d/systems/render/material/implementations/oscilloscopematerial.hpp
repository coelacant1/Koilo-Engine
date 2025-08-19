#pragma once

#include <cstddef>

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
 * @brief Material that visualizes a B-sample signal as an oscilloscope trace with N-color spectrum.
 *
 * @tparam N Number of spectrum keys.
 * @tparam B Number of audio/signal samples processed per update.
 */

/**
 * @class OscilloscopeMaterial
 * @brief Stateful material that owns smoothing filters and updates min/max/midPoint each frame.
 *
 * Exposes setters for size/position/rotation/hue/line thickness/spectrum. The input `samples`
 * pointer is external and non-owning; bind via @ref BindSamples.
 */
template <size_t N = 6, size_t B = 128>
class OscilloscopeMaterial
    : public MaterialT<OscilloscopeParamsT<N, B>, OscilloscopeShaderT<N, B>> {

    using Base = MaterialT<OscilloscopeParamsT<N, B>, OscilloscopeShaderT<N, B>>;

public:
    using Base::Base; ///< Allow aggregate-init if desired.

    /** @brief Default constructor. */
    OscilloscopeMaterial() : Base() {}

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

    /** @brief Replace the entire N-key spectrum. */
    void SetSpectrum(const RGBColor (&colors)[N]) {
        for (size_t i = 0; i < N; ++i) this->spectrum[i] = colors[i];
    }

    /** @brief Set a single spectrum key (index clamped to [0..N-1]). */
    void SetSpectrumAt(size_t i, const RGBColor& c) {
        if (i >= N) i = N - 1;
        this->spectrum[i] = c;
    }

    /** @brief Get a single spectrum key (index clamped to [0..N-1]). */
    RGBColor GetSpectrumAt(size_t i) const {
        if (i >= N) i = N - 1;
        return this->spectrum[i];
    }

    /** @brief Mutable pointer to the spectrum array (size N). */
    RGBColor*       SpectrumData()       { return this->spectrum; }

    /** @brief Const pointer to the spectrum array (size N). */
    const RGBColor* SpectrumData() const { return this->spectrum; }

    // ---------- data feed ----------

    /**
     * @brief Bind external pointer to B floats (non-owning).
     * @param samplesPtr Pointer to an array of B samples; must remain valid during use.
     */
    void BindSamples(const float* samplesPtr) { this->samples = samplesPtr; }

    /**
     * @brief Update min/max/midPoint smoothing from a representative bin.
     * @param binIndex Source sample index (default is middle bin); clamped to [0..B-1].
     *
     * Uses internal MinFilter/MaxFilter to smooth dynamic range and updates:
     * - @ref minValue
     * - @ref maxValue
     * - @ref midPoint  (average of smoothed min/max)
     */
    void UpdateScaling(size_t binIndex = B / 2) {
        if (!this->samples) return;
        if (binIndex >= B) binIndex = B - 1;

        const float v = this->samples[binIndex];

        const float minFv = minF_.Filter(v);
        const float maxFv = maxF_.Filter(v);

        this->minValue = minFv;
        this->maxValue = maxFv;
        this->midPoint = (maxFv - minFv) * 0.5f + minFv;
    }

private:
    /** @brief Smoothing filters (window size 40) for dynamic range tracking. */
    MaxFilter<40> maxF_;
    MinFilter<40> minF_;
};
