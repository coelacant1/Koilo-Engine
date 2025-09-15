#pragma once

#include <cstddef>
#include <cmath>
#include "../../../../registry/reflect_macros.hpp"

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
 * @tparam NNoise Number of noise spectrum keys.
 * @tparam NScan  Number of scanline spectrum keys.
 */

/**
 * @class TVStaticMaterial
 * @brief Stateful material that animates static/noise, scanlines, and color bars.
 *
 * Inherits @ref MaterialT to mix in the parameter block (@ref TVStaticParamsT) and bind
 * the shader singleton (@ref TVStaticShaderT). Call @ref Update(float) once per frame
 * with a monotonically increasing ratio in [0..1] (wrapping as desired) to drive animation.
 *
 * @tparam NNoise Number of noise spectrum keys.
 * @tparam NScan  Number of scanline spectrum keys.
 */
template <std::size_t NNoise = 5, std::size_t NScan = 10>
class TVStaticMaterial
  : public MaterialT<TVStaticParamsT<NNoise, NScan>, TVStaticShaderT<NNoise, NScan>> {

    using Base = MaterialT<TVStaticParamsT<NNoise, NScan>, TVStaticShaderT<NNoise, NScan>>;

public:
    /** @brief Construct with default animated signal parameters and initial scanline rotation. */
    TVStaticMaterial()
    : Base(),
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

    /** @brief Mutable pointer to noise spectrum array (size NNoise). */
    RGBColor*       NoiseSpectrum()       { return this->noiseSpectrum; }

    /** @brief Const pointer to noise spectrum array (size NNoise). */
    const RGBColor* NoiseSpectrum() const { return this->noiseSpectrum; }

    /** @brief Mutable pointer to scanline spectrum array (size NScan). */
    RGBColor*       ScanSpectrum()        { return this->scanSpectrum; }

    /** @brief Const pointer to scanline spectrum array (size NScan). */
    const RGBColor* ScanSpectrum()  const { return this->scanSpectrum; }

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

    /* NOTE: TVStaticMaterial is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(TVStaticMaterial)
        /* TODO: PTX_FIELD(TVStaticMaterial, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(TVStaticMaterial)
        PTX_METHOD_AUTO(TVStaticMaterial, SetNoiseGradientPeriod, "Set noise gradient period"),
        PTX_METHOD_AUTO(TVStaticMaterial, SetNoiseHue, "Set noise hue"),
        PTX_METHOD_AUTO(TVStaticMaterial, SetNoiseScale, "Set noise scale"),
        PTX_METHOD_AUTO(TVStaticMaterial, SetScanGradientPeriod, "Set scan gradient period"),
        PTX_METHOD_AUTO(TVStaticMaterial, SetScanMultiplyOpacity, "Set scan multiply opacity"),
        PTX_METHOD_AUTO(TVStaticMaterial, SetScanShift, "Set scan shift"),
        PTX_METHOD_AUTO(TVStaticMaterial, SetBarsCenter, "Set bars center"),
        PTX_METHOD_AUTO(TVStaticMaterial, SetBarsSize, "Set bars size"),
        PTX_METHOD_AUTO(TVStaticMaterial, SetBarsHue, "Set bars hue"),
        PTX_METHOD_AUTO(TVStaticMaterial, NoiseSpectrum, "Noise spectrum"),
        PTX_METHOD_AUTO(TVStaticMaterial, NoiseSpectrum, "Noise spectrum"),
        PTX_METHOD_AUTO(TVStaticMaterial, ScanSpectrum, "Scan spectrum"),
        PTX_METHOD_AUTO(TVStaticMaterial, ScanSpectrum, "Scan spectrum"),
        PTX_METHOD_AUTO(TVStaticMaterial, Update, "Update")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(TVStaticMaterial)
        PTX_CTOR0(TVStaticMaterial)
    PTX_END_DESCRIBE(TVStaticMaterial)
    
};
