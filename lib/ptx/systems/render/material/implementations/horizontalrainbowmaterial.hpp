#pragma once

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"
#include "../../../../registry/reflect_macros.hpp"

#include "../../shader/implementations/horizontalrainbowparams.hpp"
#include "../../shader/implementations/horizontalrainbowshader.hpp"

#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include <cstddef>

/**
 * @file horizontalrainbowmaterial.hpp
 * @brief MaterialT wrapper that renders a horizontally scrolling N-stop rainbow gradient.
 *
 * @tparam N Number of spectrum keys in the gradient.
 */

/**
 * @class HorizontalRainbowMaterial
 * @brief Stateful material that scrolls a horizontal rainbow by advancing position each update.
 *
 * Uses @c HorizontalRainbowParamsT / @c HorizontalRainbowShaderT. Call @ref Update once per frame
 * to advance the gradient by @ref scrollSpeed along +X. Spectrum and transform parameters are exposed
 * via setters/getters.
 */
template <std::size_t N = 6>
class HorizontalRainbowMaterial
    : public MaterialT<HorizontalRainbowParamsT<N>, HorizontalRainbowShaderT<N>> {

    using Base = MaterialT<HorizontalRainbowParamsT<N>, HorizontalRainbowShaderT<N>>;

public:
    using Base::Base;

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
     * @brief Replace the entire N-key spectrum.
     * @param colors Array of N RGBColor keys.
     */
    void SetSpectrum(const RGBColor (&colors)[N]) {
        for (std::size_t i = 0; i < N; ++i) this->spectrum[i] = colors[i];
    }

    /**
     * @brief Set a single spectrum key (index clamped to [0..N-1]).
     * @param i Index.
     * @param c Color value.
     */
    void SetSpectrumAt(std::size_t i, const RGBColor& c) {
        if (i >= N) i = N - 1;
        this->spectrum[i] = c;
    }

    /**
     * @brief Get a single spectrum key (index clamped to [0..N-1]).
     * @param i Index.
     * @return Color value.
     */
    RGBColor GetSpectrumAt(std::size_t i) const {
        if (i >= N) i = N - 1;
        return this->spectrum[i];
    }

    /** @brief Mutable pointer to the spectrum array (size N). */
    RGBColor*       SpectrumData()       { return this->spectrum; }

    /** @brief Const pointer to the spectrum array (size N). */
    const RGBColor* SpectrumData() const { return this->spectrum; }

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

    /* NOTE: HorizontalRainbowMaterial is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(HorizontalRainbowMaterial)
        /* TODO: PTX_FIELD(HorizontalRainbowMaterial, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(HorizontalRainbowMaterial)
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, Update, "Update"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, SetSpectrum, "Set spectrum"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, SetSpectrumAt, "Set spectrum at"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, GetSpectrumAt, "Get spectrum at"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, SpectrumData, "Spectrum data"),
        PTX_METHOD_AUTO(HorizontalRainbowMaterial, SpectrumData, "Spectrum data"),
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
        /* TODO: PTX_CTOR0(HorizontalRainbowMaterial) or PTX_CTOR(HorizontalRainbowMaterial, ...) */
    PTX_END_DESCRIBE(HorizontalRainbowMaterial)
    
};
