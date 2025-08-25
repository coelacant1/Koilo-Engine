#pragma once

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

// Templated params/shader
#include "../../shader/implementations/proceduralnoiseparams.hpp"   // ProceduralNoiseParamsT<N>
#include "../../shader/implementations/proceduralnoiseshader.hpp"   // ProceduralNoiseShaderT<N>

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector3d.hpp"
#include <cstddef>
#include <cstdint>

/**
 * @file proceduralnoisematerial.hpp
 * @brief MaterialT wrapper pairing ProceduralNoiseParamsT<N> with ProceduralNoiseShaderT<N>.
 *
 * Exposes spectrum configuration, noise scale, simplex depth (time slice), gradient period,
 * and hue shift controls.
 *
 * @tparam N Number of spectrum keys used by the shader.
 */

/**
 * @class ProceduralNoiseMaterial
 * @brief Stateful material controlling a procedural noise shader with an N-key spectrum.
 *
 * Inherits @c MaterialT to bind the shader singleton and embed the parameter block
 * (@c ProceduralNoiseParamsT<N>). Provides ergonomic setters/getters for the exposed parameters.
 *
 * @tparam N Number of spectrum keys.
 */
template <std::size_t N>
class ProceduralNoiseMaterial
    : public MaterialT<ProceduralNoiseParamsT<N>, ProceduralNoiseShaderT<N>> {
    using Base   = MaterialT<ProceduralNoiseParamsT<N>, ProceduralNoiseShaderT<N>>;
    using Params = ProceduralNoiseParamsT<N>;

public:
    using Base::Base; // allow aggregate initialization if desired

    // ----- Spectrum -----

    /**
     * @brief Replace the entire N-key spectrum.
     * @param colors Array of N colors.
     */
    void SetSpectrum(const RGBColor (&colors)[N]) {
        for (std::size_t i = 0; i < N; ++i) this->spectrum[i] = colors[i];
    }

    /**
     * @brief Set a single spectrum key (index clamped to [0..N-1]).
     * @param i Index.
     * @param c Color value.
     */
    void SetSpectrum(std::size_t i, const RGBColor& c) {
        if (i >= N) i = N - 1;
        this->spectrum[i] = c;
    }

    /**
     * @brief Get a single spectrum key (index clamped to [0..N-1]).
     * @param i Index.
     * @return Color value.
     */
    RGBColor GetSpectrum(std::size_t i) const {
        if (i >= N) i = N - 1;
        return this->spectrum[i];
    }

    /** @brief Mutable pointer to the spectrum array (size N). */
    RGBColor* SpectrumData()             { return this->spectrum; }

    /** @brief Const pointer to the spectrum array (size N). */
    const RGBColor* SpectrumData() const { return this->spectrum; }

    // ----- Noise scale (frequency per axis) -----

    /** @brief Set 3D noise frequency scaling per axis. */
    void SetNoiseScale(const Vector3D& s) { this->noiseScale = s; }

    /** @brief Get 3D noise frequency scaling per axis. */
    Vector3D GetNoiseScale() const        { return this->noiseScale; }

    // ----- Simplex Z slice (time) -----

    /** @brief Set simplex depth (interpreted as time slice). */
    void SetSimplexDepth(float d) { this->simplexDepth = d; }

    /** @brief Get simplex depth (time slice). */
    float GetSimplexDepth() const { return this->simplexDepth; }

    // ----- Gradient repetition period -----

    /** @brief Set gradient repetition period. */
    void SetGradientPeriod(float p) { this->gradientPeriod = p; }

    /** @brief Get gradient repetition period. */
    float GetGradientPeriod() const { return this->gradientPeriod; }

    // ----- Hue shift (degrees) -----

    /** @brief Set hue shift angle in degrees. */
    void SetHueShiftAngle(float deg) { this->hueShiftAngleDeg = deg; }

    /** @brief Get hue shift angle in degrees. */
    float GetHueShiftAngle() const   { return this->hueShiftAngleDeg; }
};
