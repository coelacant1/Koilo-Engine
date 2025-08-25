#pragma once

#include <cstddef>

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/spectrumanalyzerparams.hpp"
#include "../../shader/implementations/spectrumanalyzershader.hpp"

#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/control/bouncephysics.hpp"

/**
 * @file spectrumanalyzermaterial.hpp
 * @brief Material that visualizes a B-bin spectrum with an N-color gradient and optional bounce smoothing.
 *
 * @tparam N Number of spectrum keys (gradient control points).
 * @tparam B Number of spectrum bins processed per frame.
 */

/**
 * @class SpectrumAnalyzerMaterial
 * @brief Stateful material that manages per-bin BouncePhysics and exposes full configuration.
 *
 * Binds an external samples buffer (non-owning) and, on update, writes either smoothed
 * values (bounce enabled) or raw samples into @c bounceData for the shader.
 */
template <std::size_t N = 6, std::size_t B = 128>
class SpectrumAnalyzerMaterial
    : public MaterialT<SpectrumAnalyzerParamsT<N, B>, SpectrumAnalyzerShaderT<N, B>> {

    using Base = MaterialT<SpectrumAnalyzerParamsT<N, B>, SpectrumAnalyzerShaderT<N, B>>;

public:
    using Base::Base;

    /** @brief Default-construct; springs are allocated on demand when bounce is enabled. */
    SpectrumAnalyzerMaterial() : Base() {
        for (std::size_t i = 0; i < B; ++i) springs_[i] = nullptr;
    }

    /** @brief Destroy and free any allocated BouncePhysics instances. */
    ~SpectrumAnalyzerMaterial() override {
        for (std::size_t i = 0; i < B; ++i) { delete springs_[i]; springs_[i] = nullptr; }
    }

    // ---------- config: transform & shaping ----------

    /** @brief Set half-size (extent from center). */
    void SetSizeHalf(const Vector2D& half)   { this->sizeHalf = half; }

    /** @brief Set full size; internally converted to half-size. */
    void SetSizeFull(const Vector2D& full)   { this->sizeHalf = full / 2.0f; }

    /** @brief Set center offset. */
    void SetOffset(const Vector2D& c)        { this->offset   = c; }

    /** @brief Set rotation in degrees. */
    void SetRotationDeg(float deg)           { this->angleDeg = deg; }

    /** @brief Set base hue in degrees. */
    void SetHueDeg(float deg)                { this->hueDeg   = deg; }

    /** @brief Mirror the trace about the X axis. */
    void SetMirrorY(bool on)                 { this->mirrorY  = on; }
    /** @brief Get mirror-Y flag. */
    bool GetMirrorY() const                  { return this->mirrorY; }

    /** @brief Flip Y direction. */
    void SetFlipY(bool on)                   { this->flipY    = on; }
    /** @brief Get flip-Y flag. */
    bool GetFlipY() const                    { return this->flipY; }

    /** @brief Set vertical scale multiplier for bars/curve. */
    void SetHeightScale(float s)             { this->heightScale = s; }
    /** @brief Get vertical scale multiplier. */
    float GetHeightScale() const             { return this->heightScale; }

    /**
     * @brief Toggle per-bin bounce smoothing.
     * @details Allocates @ref BouncePhysics when enabling; frees when disabling to save memory.
     */
    void SetBounceEnabled(bool on) {
        if (this->bounce == on) return;
        this->bounce = on;
        if (on) {
            for (std::size_t i = 0; i < B; ++i)
                if (!springs_[i]) springs_[i] = new BouncePhysics(35.0f, 15.0f);
        } else {
            for (std::size_t i = 0; i < B; ++i) { delete springs_[i]; springs_[i] = nullptr; }
        }
    }
    /** @brief Query bounce-smoothing state. */
    bool GetBounceEnabled() const            { return this->bounce; }

    // ---------- gradient config ----------

    /** @brief Replace the entire N-key spectrum. */
    void SetSpectrum(const RGBColor (&colors)[N]) {
        for (std::size_t i = 0; i < N; ++i) this->spectrum[i] = colors[i];
    }

    /** @brief Set a single spectrum key (index clamped to [0..N-1]). */
    void SetSpectrumAt(std::size_t i, const RGBColor& c) {
        if (i >= N) i = N - 1;
        this->spectrum[i] = c;
    }

    /** @brief Get a single spectrum key (index clamped to [0..N-1]). */
    RGBColor GetSpectrumAt(std::size_t i) const {
        if (i >= N) i = N - 1;
        return this->spectrum[i];
    }

    /** @brief Mutable pointer to the spectrum array (size N). */
    RGBColor*       SpectrumData()       { return this->spectrum; }

    /** @brief Const pointer to the spectrum array (size N). */
    const RGBColor* SpectrumData() const { return this->spectrum; }

    // ---------- data binding / update ----------

    /** @brief Bind external pointer to B floats (non-owning). */
    void BindSamples(const float* samplesPtr) { this->samples = samplesPtr; }

    /**
     * @brief Per-frame update of bounceData from input samples.
     * @param readData Optional pointer to B samples; if null uses previously bound samples.
     *
     * Writes smoothed values when bounce is enabled; otherwise mirrors raw samples.
     * The shader consumes @c bounceData regardless of mode.
     */
    void Update(const float* readData = nullptr) override {
        if (readData) this->samples = readData;
        if (!this->samples) return;

        if (this->bounce) {
            for (std::size_t i = 0; i < B; ++i) {
                const float in = this->samples[i];
                this->bounceData[i] = springs_[i] ? springs_[i]->Calculate(in, 0.1f) : in;
            }
        } else {
            for (std::size_t i = 0; i < B; ++i) this->bounceData[i] = this->samples[i];
        }
    }

private:
    BouncePhysics* springs_[B]; ///< Allocated only when bounce is enabled.
};
