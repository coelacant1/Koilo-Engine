#pragma once

#include <cstddef>

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
 * @brief Audio-reactive gradient material that blends N spectrum keys and (optionally) applies bounce physics to B samples.
 *
 * @details
 * - Owns @ref BouncePhysics instances when bounce is enabled; writes @c bounceData[] in @ref Update().
 * - Exposes setters/getters for size, position, rotation, hue, circular flag, radius, bounce, and spectrum keys.
 * - @c samples pointer is external and non-owning; bind via @ref BindSamples().
 *
 * @tparam N Number of spectrum keys (gradient control points).
 * @tparam B Number of audio samples processed per frame.
 */

/**
 * @class AudioReactiveMaterial
 * @brief Stateful material that reacts to an external B-sample buffer and forwards shading to @c AudioReactiveShaderT.
 *
 * Inherits @c MaterialT to mix in the parameter block (@c AudioReactiveParamsT) and bind the shader singleton
 * (@c AudioReactiveShaderT). If bounce is enabled, each sample channel is processed by a @c BouncePhysics instance
 * before being written into @c bounceData[] for the shader to consume.
 */
template <std::size_t N = 6, std::size_t B = 128>
class AudioReactiveMaterial
    : public MaterialT<AudioReactiveParamsT<N, B>, AudioReactiveShaderT<N, B>> {

    using Base = MaterialT<AudioReactiveParamsT<N, B>, AudioReactiveShaderT<N, B>>;

public:
    using Base::Base;

    /** @brief Default-construct; bounce physics are lazily allocated if/when enabled. */
    AudioReactiveMaterial()
        : Base() {
        for (std::size_t i = 0; i < B; ++i) bPhy_[i] = nullptr;
    }

    /** @brief Frees any allocated @ref BouncePhysics instances. */
    ~AudioReactiveMaterial() override {
        for (std::size_t i = 0; i < B; ++i) { delete bPhy_[i]; bPhy_[i] = nullptr; }
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
     * @details Allocates @ref BouncePhysics when turning on; frees them when turning off to save RAM.
     */
    void SetBounceEnabled(bool on) {
        if (this->bounce == on) return;
        this->bounce = on;
        if (on) {
            for (std::size_t i = 0; i < B; ++i) {
                if (!bPhy_[i]) bPhy_[i] = new BouncePhysics(35.0f, 15.0f);
            }
        } else {
            for (std::size_t i = 0; i < B; ++i) { delete bPhy_[i]; bPhy_[i] = nullptr; }
        }
    }

    /** @brief Query whether bounce physics are enabled. */
    bool GetBounceEnabled() const { return this->bounce; }

    /** @brief Replace the entire N-key spectrum. */
    void SetSpectrum(const RGBColor (&colors)[N]) {
        for (std::size_t i = 0; i < N; ++i) this->spectrum[i] = colors[i];
    }

    /** @brief Set a single spectrum key (clamped to [0..N-1]). */
    void SetSpectrumAt(std::size_t i, const RGBColor& c) {
        if (i >= N) i = N - 1;
        this->spectrum[i] = c;
    }

    /** @brief Get a single spectrum key (clamped to [0..N-1]). */
    RGBColor GetSpectrumAt(std::size_t i) const {
        if (i >= N) i = N - 1;
        return this->spectrum[i];
    }

    /** @brief Mutable pointer to the spectrum array (size N). */
    RGBColor*       SpectrumData()       { return this->spectrum; }

    /** @brief Const pointer to the spectrum array (size N). */
    const RGBColor* SpectrumData() const { return this->spectrum; }

    /**
     * @brief Bind an external, non-owning pointer to B audio samples.
     * @param samplesPtr Pointer to an array of B floats; must remain valid while used.
     */
    void BindSamples(const float* samplesPtr) { this->samples = samplesPtr; }

    /**
     * @brief Per-frame update; optionally supply fresh sample data.
     * @param readData Optional pointer to B samples; if nullptr uses previously bound @ref samples.
     *
     * If bounce is enabled, each channel runs through @ref BouncePhysics::Calculate with a damping factor
     * (0.1f) before being written to @c bounceData[]. Otherwise, raw samples are mirrored directly.
     *
     * @note The shader reads a single path (@c bounceData) regardless of bounce mode.
     */
    void Update(float dT) override {
        if (this->bounce) {
            for (std::size_t i = 0; i < B; ++i) {
                const float in = this->samples[i];
                this->bounceData[i] = bPhy_[i] ? bPhy_[i]->Calculate(in, 0.1f) : in;
            }
        } else {
            for (std::size_t i = 0; i < B; ++i) this->bounceData[i] = this->samples[i];
        }
    }

private:
    BouncePhysics* bPhy_[B]; ///< Lazily allocated per channel when bounce is enabled.

};
