#pragma once

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/horizontalrainbowparams.hpp"
#include "../../shader/implementations/horizontalrainbowshader.hpp"

#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../registry/reflect_macros.hpp"
#include <algorithm>
#include <cstddef>
#include <vector>

/**
 * @file horizontalrainbowmaterial.hpp
 * @brief MaterialT wrapper that renders a horizontally scrolling rainbow gradient.
 */

class HorizontalRainbowMaterial
    : public MaterialT<HorizontalRainbowParams, HorizontalRainbowShader> {

public:
    explicit HorizontalRainbowMaterial(std::size_t spectrumCount = 6)
        : MaterialT<HorizontalRainbowParams, HorizontalRainbowShader>(spectrumCount) {}

    /**
     * @brief Per-frame animation hook; advances the gradient horizontally.
     * @param dtOrRatio Time step or normalized ratio (unused here).
     * @details Increments @ref positionOffset.X by @ref scrollSpeed each call.
     */
    void Update(float /*dtOrRatio*/) override {
        this->positionOffset.X += this->scrollSpeed;
    }

    // ---- Spectrum configuration ----

    /**
     * @brief Replace the entire spectrum from a std::vector.
     */
    void SetSpectrum(const std::vector<RGBColor>& colors) {
        this->ResizeSpectrum(colors.size());
        std::copy(colors.begin(), colors.end(), this->spectrum.begin());
    }

    /**
     * @brief Replace the entire spectrum from a fixed array.
     */
    template<std::size_t N>
    void SetSpectrumFromArray(const RGBColor (&colors)[N]) {
        this->ResizeSpectrum(N);
        for (std::size_t i = 0; i < N; ++i) {
            this->spectrum[i] = colors[i];
        }
    }

    /**
     * @brief Set the active spectrum count (resizes storage and seeds defaults).
     */
    void SetSpectrumCount(std::size_t count) { this->ResizeSpectrum(count); }

    /**
     * @brief Set a single spectrum key (index clamped to valid range).
     */
    void SetSpectrumAt(std::size_t i, const RGBColor& c) {
        if (this->spectrum.empty()) return;
        if (i >= this->spectrum.size()) i = this->spectrum.size() - 1;
        this->spectrum[i] = c;
    }

    /**
     * @brief Get a single spectrum key (index clamped to valid range).
     */
    RGBColor GetSpectrumAt(std::size_t i) const {
        if (this->spectrum.empty()) return RGBColor();
        if (i >= this->spectrum.size()) i = this->spectrum.size() - 1;
        return this->spectrum[i];
    }

    /** @brief Mutable pointer to the spectrum array (may be null when empty). */
    RGBColor* SpectrumData() { return this->spectrum.empty() ? nullptr : this->spectrum.data(); }

    /** @brief Const pointer to the spectrum array (may be null when empty). */
    const RGBColor* SpectrumData() const { return this->spectrum.empty() ? nullptr : this->spectrum.data(); }

    /** @brief Query the number of active spectrum entries. */
    [[nodiscard]] std::size_t SpectrumCount() const { return this->HorizontalRainbowParams::SpectrumCount(); }

    // ---- Transform/period/scroll configuration ----

    /** @brief Set gradient position offset. */
    void SetPositionOffset(const Vector2D& o) { this->positionOffset = o; }
    /** @brief Get gradient position offset. */
    Vector2D GetPositionOffset() const        { return this->positionOffset; }

    /** @brief Set rotation in degrees. */
    void SetRotationDeg(float deg)            { this->rotationDeg = deg; }
    /** @brief Get rotation in degrees. */
    float GetRotationDeg() const              { return this->rotationDeg; }

    /** @brief Set gradient period. */
    void SetGradientPeriod(float p)           { this->gradientPeriod = p; }
    /** @brief Get gradient period. */
    float GetGradientPeriod() const           { return this->gradientPeriod; }

    /** @brief Set scroll speed (delta X per @ref Update call). */
    void SetScrollSpeed(float s)              { this->scrollSpeed = s; }
    /** @brief Get scroll speed. */
    float GetScrollSpeed() const              { return this->scrollSpeed; }

    PTX_BEGIN_FIELDS(HorizontalRainbowMaterial)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(HorizontalRainbowMaterial)
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, Update, "Update"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, SetSpectrum, "Set spectrum"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, SetSpectrumCount, "Set spectrum count"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, SetSpectrumAt, "Set spectrum at"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, GetSpectrumAt, "Get spectrum at"),
        /* Spectrum data */ PTX_METHOD_OVLD0(HorizontalRainbowMaterial, SpectrumData, RGBColor *),
        /* Spectrum data */ PTX_METHOD_OVLD_CONST0(HorizontalRainbowMaterial, SpectrumData, const RGBColor *),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, SpectrumCount, "Spectrum count"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, SetPositionOffset, "Set position offset"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, GetPositionOffset, "Get position offset"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, SetRotationDeg, "Set rotation deg"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, GetRotationDeg, "Get rotation deg"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, SetGradientPeriod, "Set gradient period"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, GetGradientPeriod, "Get gradient period"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, SetScrollSpeed, "Set scroll speed"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, GetScrollSpeed, "Get scroll speed")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(HorizontalRainbowMaterial)
        PTX_CTOR(HorizontalRainbowMaterial, std::size_t)
    PTX_END_DESCRIBE(HorizontalRainbowMaterial)
};
