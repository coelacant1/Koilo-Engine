#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include "../../../../registry/reflect_macros.hpp"

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/audioreactiveparams.hpp"
#include "../../shader/implementations/audioreactiveshader.hpp"

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/control/bouncephysics.hpp"

/**
 * @file audioreactivematerial.hpp
 * @brief Audio-reactive gradient material with runtime-sized spectrum keys and sample buffers.
 *
 * @details
 * - Owns @ref BouncePhysics instances when bounce is enabled; writes @c bounceData in @ref Update().
 * - Exposes setters/getters for size, position, rotation, hue, circular flag, radius, bounce, and spectrum keys.
 * - @c samples pointer is external and non-owning; bind via @ref BindSamples().
 */

class AudioReactiveMaterial : public MaterialT<AudioReactiveParams, AudioReactiveShader> {
public:
    explicit AudioReactiveMaterial(std::size_t spectrumCount = 6, std::size_t sampleCount = 128)
        : MaterialT<AudioReactiveParams, AudioReactiveShader>(spectrumCount, sampleCount) {
        bouncePhysics_.resize(this->SampleCount());
    }

    /** @brief Set half-size (logical extent from center). */
    void SetSizeHalf(const Vector2D& half)  { this->sizeHalf = half; }

    /** @brief Set full size; internally converted to half-size. */
    void SetSizeFull(const Vector2D& full)  { this->sizeHalf = full / 2.0f; }

    /** @brief Set the center offset. */
    void SetOffset(const Vector2D& c)       { this->offset   = c; }

    /** @brief Set rotation in degrees. */
    void SetRotationDeg(float deg)          { this->angleDeg = deg; }

    /** @brief Set base hue (degrees). */
    void SetHueDeg(float deg)               { this->hueDeg   = deg; }

    /** @brief Enable/disable circular rendering mode. */
    void SetCircular(bool c)                { this->circular = c; }

    /** @brief Query circular rendering mode. */
    bool GetCircular() const                { return this->circular; }

    /** @brief Set circle radius (used when @ref GetCircular is true). */
    void SetRadius(float r)                 { this->radius = r; }

    /** @brief Get circle radius. */
    float GetRadius() const                 { return this->radius; }

    /**
     * @brief Toggle per-channel bounce physics.
     * @details Allocates @ref BouncePhysics when turning on; releases instances when turning off.
     */
    void SetBounceEnabled(bool on) {
        if (this->bounce == on) return;
        this->bounce = on;
        if (on) {
            EnsureBounceCapacity();
            for (auto& phy : bouncePhysics_) {
                if (!phy) phy = std::make_unique<BouncePhysics>(35.0f, 15.0f);
            }
        } else {
            for (auto& phy : bouncePhysics_) {
                phy.reset();
            }
        }
    }

    /** @brief Query whether bounce physics are enabled. */
    bool GetBounceEnabled() const { return this->bounce; }

    /** @brief Resize the spectrum and optionally seed from an array. */
    void SetSpectrumCount(std::size_t count) { this->ResizeSpectrum(count); }

    /** @brief Replace the entire spectrum with values from a vector. */
    void SetSpectrum(const std::vector<RGBColor>& colors) {
        this->ResizeSpectrum(colors.size());
        std::copy(colors.begin(), colors.end(), this->spectrum.begin());
    }

    /** @brief Replace the entire spectrum with values from a fixed array. */
    template<std::size_t N>
    void SetSpectrumFromArray(const RGBColor (&colors)[N]) {
        this->ResizeSpectrum(N);
        for (std::size_t i = 0; i < N; ++i) {
            this->spectrum[i] = colors[i];
        }
    }

    /** @brief Set a single spectrum key (clamped to valid range). */
    void SetSpectrumAt(std::size_t i, const RGBColor& c) {
        if (this->spectrum.empty()) return;
        if (i >= this->spectrum.size()) i = this->spectrum.size() - 1;
        this->spectrum[i] = c;
    }

    /** @brief Get a single spectrum key (clamped to valid range). */
    RGBColor GetSpectrumAt(std::size_t i) const {
        if (this->spectrum.empty()) return RGBColor();
        if (i >= this->spectrum.size()) i = this->spectrum.size() - 1;
        return this->spectrum[i];
    }

    /** @brief Mutable pointer to the spectrum array (may be null when empty). */
    RGBColor* SpectrumData() { return this->spectrum.empty() ? nullptr : this->spectrum.data(); }

    /** @brief Const pointer to the spectrum array (may be null when empty). */
    const RGBColor* SpectrumData() const { return this->spectrum.empty() ? nullptr : this->spectrum.data(); }

    /** @brief Set the expected number of spectrum samples (bins). */
    void SetSampleCount(std::size_t sampleCount) {
        this->ResizeSamples(sampleCount);
        bouncePhysics_.resize(sampleCount);
        if (this->bounce) {
            for (auto& phy : bouncePhysics_) {
                if (!phy) phy = std::make_unique<BouncePhysics>(35.0f, 15.0f);
            }
        }
    }

    /**
     * @brief Bind an external, non-owning pointer to audio samples.
     * @param samplesPtr Pointer to @ref SampleCount entries; must remain valid while used.
     */
    void BindSamples(const float* samplesPtr) { this->samples = samplesPtr; }

    /** @brief Bind samples and resize storage in a single call. */
    void BindSamples(const float* samplesPtr, std::size_t sampleCount) {
        BindSamples(samplesPtr);
        SetSampleCount(sampleCount);
    }

    /**
     * @brief Per-frame update; optionally supply fresh sample data via @ref BindSamples.
     */
    void Update(float dT) override {
        const std::size_t count = this->SampleCount();
        if (count == 0) return;

        const float* src = this->samples;
        if (!src) {
            std::fill(this->bounceData.begin(), this->bounceData.end(), 0.0f);
            return;
        }

        if (this->bounce) {
            EnsureBounceCapacity();
            const float dt = dT > 0.0f ? dT : 0.1f;
            for (std::size_t i = 0; i < count; ++i) {
                auto& phy = bouncePhysics_[i];
                if (!phy) {
                    phy = std::make_unique<BouncePhysics>(35.0f, 15.0f);
                }
                this->bounceData[i] = phy->Calculate(src[i], dt);
            }
        } else {
            std::copy(src, src + count, this->bounceData.begin());
        }
    }

    PTX_BEGIN_FIELDS(AudioReactiveMaterial)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(AudioReactiveMaterial)
        PTX_METHOD_AUTO(AudioReactiveMaterial, SetSizeHalf, "Set size half"),
        PTX_METHOD_AUTO(AudioReactiveMaterial, SetSizeFull, "Set size full"),
        PTX_METHOD_AUTO(AudioReactiveMaterial, SetOffset, "Set offset"),
        PTX_METHOD_AUTO(AudioReactiveMaterial, SetRotationDeg, "Set rotation deg"),
        PTX_METHOD_AUTO(AudioReactiveMaterial, SetHueDeg, "Set hue deg"),
        PTX_METHOD_AUTO(AudioReactiveMaterial, SetCircular, "Set circular"),
        PTX_METHOD_AUTO(AudioReactiveMaterial, GetCircular, "Get circular"),
        PTX_METHOD_AUTO(AudioReactiveMaterial, SetRadius, "Set radius"),
        PTX_METHOD_AUTO(AudioReactiveMaterial, GetRadius, "Get radius"),
        PTX_METHOD_AUTO(AudioReactiveMaterial, SetBounceEnabled, "Set bounce enabled"),
        PTX_METHOD_AUTO(AudioReactiveMaterial, GetBounceEnabled, "Get bounce enabled"),
        PTX_METHOD_AUTO(AudioReactiveMaterial, SetSpectrumCount, "Set spectrum count"),
        PTX_METHOD_OVLD(AudioReactiveMaterial, SetSpectrum, void, const std::vector<RGBColor> &),
        PTX_METHOD_AUTO(AudioReactiveMaterial, SetSpectrumAt, "Set spectrum at"),
        PTX_METHOD_AUTO(AudioReactiveMaterial, GetSpectrumAt, "Get spectrum at"),
        /* Spectrum data */ PTX_METHOD_OVLD0(AudioReactiveMaterial, SpectrumData, RGBColor *),
        /* Spectrum data */ PTX_METHOD_OVLD_CONST0(AudioReactiveMaterial, SpectrumData, const RGBColor *),
        PTX_METHOD_AUTO(AudioReactiveMaterial, SetSampleCount, "Set sample count"),
        PTX_METHOD_OVLD(AudioReactiveMaterial, BindSamples, void, const float *),
        PTX_METHOD_OVLD(AudioReactiveMaterial, BindSamples, void, const float *, std::size_t),
        PTX_METHOD_AUTO(AudioReactiveMaterial, Update, "Update")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(AudioReactiveMaterial)
        PTX_CTOR(AudioReactiveMaterial, std::size_t, std::size_t)
    PTX_END_DESCRIBE(AudioReactiveMaterial)

private:
    void EnsureBounceCapacity() {
        if (bouncePhysics_.size() < this->SampleCount()) {
            bouncePhysics_.resize(this->SampleCount());
        }
    }

    std::vector<std::unique_ptr<BouncePhysics>> bouncePhysics_{}; ///< Lazily allocated per channel when bounce is enabled.
};
