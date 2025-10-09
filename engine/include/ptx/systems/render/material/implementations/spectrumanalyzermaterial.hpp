#pragma once

#include <cstddef>
#include <memory>
#include <vector>

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
 */

/**
 * @class SpectrumAnalyzerMaterial
 * @brief Stateful material that manages per-bin BouncePhysics and exposes full configuration.
 *
 * Binds an external samples buffer (non-owning) and, on update, writes either smoothed
 * values (bounce enabled) or raw samples into @c bounceData for the shader.
 */
class SpectrumAnalyzerMaterial
    : public MaterialT<SpectrumAnalyzerParams, SpectrumAnalyzerShader> {

    using Base = MaterialT<SpectrumAnalyzerParams, SpectrumAnalyzerShader>;

public:
    /** @brief Construct with desired spectrum key count and bin count. */
    explicit SpectrumAnalyzerMaterial(std::size_t spectrumCount = 6, std::size_t binCount = 128)
        : Base(spectrumCount, binCount) {
        springs_.resize(binCount);
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

    /** @brief Set the number of spectrum keys, optionally feeding default rainbow colors. */
    void SetSpectrumCount(std::size_t count) {
        this->ResizeSpectrum(count);
    }

    /** @brief Set the number of bins (per-frame samples). Resets bounce storage. */
    void SetBinCount(std::size_t count) {
        const std::size_t spectrumCount = this->SpectrumCount();
        this->Resize(spectrumCount, count);
        springs_.resize(count);
        if (!this->bounce) {
            for (auto& spring : springs_) spring.reset();
        } else {
            EnsureSprings();
        }
    }

    /** @brief Current spectrum key count. */
    [[nodiscard]] std::size_t SpectrumCount() const { return this->SpectrumAnalyzerParams::SpectrumCount(); }

    /** @brief Current bin/sample count. */
    [[nodiscard]] std::size_t BinCount() const { return this->SpectrumAnalyzerParams::BinCount(); }

    /**
     * @brief Toggle per-bin bounce smoothing.
     * @details Allocates @ref BouncePhysics when enabling; frees when disabling to save memory.
     */
    void SetBounceEnabled(bool on) {
        if (this->bounce == on) return;
        this->bounce = on;
        if (on) {
            EnsureSprings();
        } else {
            for (auto& spring : springs_) spring.reset();
        }
    }
    /** @brief Query bounce-smoothing state. */
    bool GetBounceEnabled() const            { return this->bounce; }

    // ---------- gradient config ----------

    /** @brief Replace the spectrum from a pointer/count pair. */
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

    /** @brief Replace the entire spectrum from a container. */
    void SetSpectrum(const std::vector<RGBColor>& colors) {
        SetSpectrum(colors.data(), colors.size());
    }

    /** @brief Replace the entire spectrum from a fixed-size array. */
    template <std::size_t Count>
    void SetSpectrum(const RGBColor (&colors)[Count]) {
        SetSpectrum(colors, Count);
    }

    /** @brief Set a single spectrum key (index clamped to [0..count-1]). */
    void SetSpectrumAt(std::size_t i, const RGBColor& c) {
        if (this->SpectrumCount() == 0) return;
        if (i >= this->SpectrumCount()) i = this->SpectrumCount() - 1;
        RGBColor* data = this->SpectrumData();
        if (data) data[i] = c;
    }

    /** @brief Get a single spectrum key (index clamped to [0..count-1]). */
    RGBColor GetSpectrumAt(std::size_t i) const {
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

    // ---------- data binding / update ----------

    /** @brief Bind external pointer to floats (non-owning). */
    void BindSamples(const float* samplesPtr) { this->samples = samplesPtr; }

    /** @brief Update via IMaterial interface (uses previously bound samples). */
    void Update(float /*deltaTime*/) override { ProcessSamples(nullptr); }

    /**
     * @brief Update bounce data using the provided samples pointer.
     * @details If @p readData is null, the last bound pointer is used.
     */
    void UpdateSamples(const float* readData = nullptr) { ProcessSamples(readData); }

private:
    void EnsureSprings() {
        const std::size_t bins = this->BinCount();
        springs_.resize(bins);
        for (std::size_t i = 0; i < bins; ++i) {
            if (!springs_[i]) {
                springs_[i] = std::make_unique<BouncePhysics>(35.0f, 15.0f);
            }
        }
    }

    void ProcessSamples(const float* readData) {
        if (readData) this->samples = readData;
        if (!this->samples || this->BinCount() == 0) return;

        float* bounceTarget = this->BounceData();
        if (!bounceTarget) {
            return;
        }

        if (this->bounce) {
            EnsureSprings();
            for (std::size_t i = 0; i < this->BinCount(); ++i) {
                const float in = this->samples[i];
                auto& spring = springs_[i];
                bounceTarget[i] = spring ? spring->Calculate(in, 0.1f) : in;
            }
        } else {
            for (std::size_t i = 0; i < this->BinCount(); ++i) {
                bounceTarget[i] = this->samples[i];
            }
        }
    }

    std::vector<std::unique_ptr<BouncePhysics>> springs_{}; ///< Allocated only when bounce is enabled.

};
