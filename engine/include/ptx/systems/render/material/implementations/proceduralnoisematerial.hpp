#pragma once

#include <cstddef>
#include <vector>

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/proceduralnoiseparams.hpp"
#include "../../shader/implementations/proceduralnoiseshader.hpp"

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector3d.hpp"

/**
 * @file proceduralnoisematerial.hpp
 * @brief MaterialT wrapper pairing ProceduralNoiseParams with ProceduralNoiseShader.
 *
 * Exposes spectrum configuration, noise scale, simplex depth (time slice), gradient period,
 * and hue shift controls with runtime-sized spectrum.
 */

/**
 * @class ProceduralNoiseMaterial
 * @brief Stateful material controlling a procedural noise shader with runtime spectrum sizing.
 *
 * Inherits @c MaterialT to bind the shader singleton and embed the parameter block
 * (@c ProceduralNoiseParams). Provides ergonomic setters/getters for the exposed parameters.
 */
class ProceduralNoiseMaterial
    : public MaterialT<ProceduralNoiseParams, ProceduralNoiseShader> {
    using Base   = MaterialT<ProceduralNoiseParams, ProceduralNoiseShader>;

public:
    /** @brief Construct with desired spectrum key count. */
    explicit ProceduralNoiseMaterial(std::size_t spectrumCount = 6)
        : Base(spectrumCount) {}

    // ----- Spectrum -----

    /** @brief Set the number of spectrum keys, optionally feeding default rainbow colors. */
    void SetSpectrumCount(std::size_t count) {
        this->ResizeSpectrum(count);
    }

    /** @brief Current spectrum key count. */
    [[nodiscard]] std::size_t SpectrumCount() const { return this->ProceduralNoiseParams::SpectrumCount(); }

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

    /**
     * @brief Set a single spectrum key (index clamped to [0..count-1]).
     * @param i Index.
     * @param c Color value.
     */
    void SetSpectrumAt(std::size_t i, const RGBColor& c) {
        if (this->SpectrumCount() == 0) return;
        if (i >= this->SpectrumCount()) i = this->SpectrumCount() - 1;
        RGBColor* data = this->SpectrumData();
        if (data) data[i] = c;
    }

    /**
     * @brief Get a single spectrum key (index clamped to [0..count-1]).
     * @param i Index.
     * @return Color value.
     */
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
