#pragma once

#include <cstddef>
#include <cmath>

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/tvstaticparams.hpp"
#include "../../shader/implementations/tvstaticshader.hpp"

#include "../../../../core/signal/functiongenerator.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

/**
 * @file tvstaticmaterial.hpp
 * @brief TV static material with animated noise, scanlines, and color bars.
 *
 * Holds animated state via @ref FunctionGenerator instances and writes animated parameters
 * into the parameter block each frame from @ref Update(float).
 *
 */

/**
 * @class TVStaticMaterial
 * @brief Stateful material that animates static/noise, scanlines, and color bars.
 *
 * Inherits @ref MaterialT to mix in the parameter block (@ref TVStaticParams) and bind
 * the shader singleton (@ref TVStaticShader). Call @ref Update(float) once per frame
 * with a monotonically increasing ratio in [0..1] (wrapping as desired) to drive animation.
 */
class TVStaticMaterial : public MaterialT<TVStaticParams, TVStaticShader> {

        using Base = MaterialT<TVStaticParams, TVStaticShader>;

public:
    /** @brief Construct with default animated signal parameters and initial scanline rotation. */
        explicit TVStaticMaterial(std::size_t noiseSpectrumCount = 5, std::size_t scanSpectrumCount = 10)
        : Base(noiseSpectrumCount, scanSpectrumCount),
      wiggle1_(FunctionGenerator::Sine, -40.0f, 20.0f, 1.95f),
      wiggle2_(FunctionGenerator::Sine, -10.0f, 10.0f, 0.87f),
      fGrad_  (FunctionGenerator::Sine,   0.0f,  0.5f, 6.65f) {
        this->scanRotationDeg = 90.0f;
    }

    // ---------------- Parameter setters ----------------

    /** @brief Set noise gradient period. */
    void SetNoiseGradientPeriod(float p) { this->noiseGradientPeriod = p; }

    /** @brief Set noise base hue in degrees. */
    void SetNoiseHue(float deg)          { this->noiseHueDeg = deg; }

    /** @brief Set noise scale (xyz). */
    void SetNoiseScale(const Vector3D& s){ this->noiseScale = s; }

    /** @brief Set scanline gradient period. */
    void SetScanGradientPeriod(float p)  { this->scanGradientPeriod = p; }

    /** @brief Set scanline multiply opacity. */
    void SetScanMultiplyOpacity(float o) { this->scanMultiplyOpacity = o; }

    /** @brief Set scanline phase shift [0..1]. */
    void SetScanShift(float s)           { this->scanShift = s; }

    /** @brief Set color bars center. */
    void SetBarsCenter(const Vector2D& c){ this->barsCenter = c; }

    /** @brief Set color bars size. */
    void SetBarsSize(const Vector2D& s)  { this->barsSize = s; }

    /** @brief Set color bars base hue in degrees. */
    void SetBarsHue(float deg)           { this->barsHueDeg = deg; }

    /** @brief Resize noise spectrum array. */
    void SetNoiseSpectrumCount(std::size_t count) { this->ResizeNoiseSpectrum(count); }

    /** @brief Resize scan spectrum array. */
    void SetScanSpectrumCount(std::size_t count)  { this->ResizeScanSpectrum(count); }

    /** @brief Number of noise spectrum keys currently allocated. */
    [[nodiscard]] std::size_t NoiseSpectrumSize() const { return TVStaticParams::NoiseSpectrumCount(); }

    /** @brief Number of scan spectrum keys currently allocated. */
    [[nodiscard]] std::size_t ScanSpectrumSize() const { return TVStaticParams::ScanSpectrumCount(); }

    /** @brief Mutable pointer to noise spectrum array (size determined at runtime). */
    RGBColor*       NoiseSpectrum()       { return this->NoiseSpectrumData(); }

    /** @brief Const pointer to noise spectrum array (size determined at runtime). */
    const RGBColor* NoiseSpectrum() const { return this->NoiseSpectrumData(); }

    /** @brief Mutable pointer to scanline spectrum array (size determined at runtime). */
    RGBColor*       ScanSpectrum()        { return this->ScanSpectrumData(); }

    /** @brief Const pointer to scanline spectrum array (size determined at runtime). */
    const RGBColor* ScanSpectrum()  const { return this->ScanSpectrumData(); }

    // ---------------- Animation update ----------------

    /**
     * @brief Per-frame animation update; writes animated parameters to the block.
     * @param ratio Monotonic time ratio in [0..1] (wrap as needed for looping).
     *
     * Advances noise depth, animates noise gradient/hue/scale, scrolls scanlines by @ref scanShift,
     * and updates color bars hue and center using two @ref FunctionGenerator signals.
     */
    void Update(float ratio) override {
        // Advance noise depth
        this->noiseZ += 0.1f;

        // Animate noise gradient + hue + scale
        this->noiseGradientPeriod = 0.5f;
        this->noiseHueDeg         = ratio * 360.0f * 2.0f;
        this->noiseScale          = Vector3D(0.5f, 0.5f, 0.5f);

        // Animate scanlines (phase shift)
        (void)fGrad_.Update();
        this->scanShift = std::fmod(ratio * 5.0f, 1.0f);

        // Animate color bars: hue + position
        this->barsHueDeg = 360.0f * ratio;
        this->barsCenter = Vector2D(
            96.0f + wiggle1_.Update(),
            Map(ratio, 0.0f, 1.0f, -100.0f, 100.0f) + wiggle2_.Update()
        );
    }

private:
    /** @brief Linear map from [a..b] to [c..d]. */
    static float Map(float x, float a, float b, float c, float d) {
        const float t = (x - a) / (b - a);
        return c + (d - c) * t;
    }

    // Animated signals
    FunctionGenerator wiggle1_;
    FunctionGenerator wiggle2_;
    FunctionGenerator fGrad_;

};
