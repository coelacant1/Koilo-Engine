#pragma once

#include <algorithm>
#include <cstddef>
#include "../../../../registry/reflect_macros.hpp"

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/gradientparams.hpp"
#include "../../shader/implementations/gradientshader.hpp"

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"

/**
 * @file gradientmaterial.hpp
 * @brief Runtime-sized gradient material pairing @ref GradientParams with @ref GradientShader.
 */
class GradientMaterial : public MaterialT<GradientParams, GradientShader> {
    using Base = MaterialT<GradientParams, GradientShader>;

public:
    /**
     * @brief Construct a gradient material with a runtime color count.
     * @param colorCount     Number of gradient keys to allocate.
     * @param basePalette    Optional pointer to an external palette used for hue shifting.
     * @param gradientPeriod Repeat distance for the gradient.
     * @param radial         True for radial gradients, false for linear.
     * @param stepped        True for stepped gradients instead of smooth interpolation.
     */
    explicit GradientMaterial(std::size_t colorCount = 6,
                              const RGBColor* basePalette = nullptr,
                              float gradientPeriod = 1.0f,
                              bool radial = false,
                              bool stepped = false)
        : Base(colorCount),
          basePalette_(basePalette) {
        this->gradientPeriod = gradientPeriod;
        this->isRadial       = radial;
        this->isStepped      = stepped;
        this->baseColors     = basePalette;
        basePaletteCount_    = basePalette ? colorCount : 0;
        if (basePalette) {
            UpdateGradient(basePalette, colorCount);
        }
    }

    /** @brief Update the translation offset. */
    void SetPositionOffset(Vector2D positionOffset) { this->positionOffset = positionOffset; }

    /** @brief Update the rotation pivot. */
    void SetRotationOffset(Vector2D rotationOffset) { this->rotationOffset = rotationOffset; }

    /** @brief Update rotation angle in degrees. */
    void SetRotationAngle(float rotationAngle)      { this->rotationAngle  = rotationAngle; }

    /** @brief Set gradient repetition period. */
    void SetGradientPeriod(float gradientPeriod)    { this->gradientPeriod = gradientPeriod; }

    /** @brief Shift gradient phase by @p ratio (0..1). */
    void GradientShift(float ratio)                 { this->gradientShift  = ratio; }

    /** @brief Resize the gradient color array. */
    void SetColorCount(std::size_t colorCount) {
        this->GradientParams::Resize(colorCount);
        if (basePalette_) {
            basePaletteCount_ = std::min<std::size_t>(basePaletteCount_, colorCount);
        } else {
            basePaletteCount_ = 0;
        }
    }

    /** @brief Number of active gradient keys. */
    [[nodiscard]] std::size_t ColorCount() const { return this->colors.size(); }

    /**
     * @brief Replace the active gradient colors from an external buffer.
     * @param colors Pointer to RGBColor entries.
     * @param count  Number of entries available at @p colors.
     */
    void UpdateGradient(const RGBColor* colors, std::size_t count) {
        if (!colors || count == 0) return;
        if (this->colors.size() != count) {
            this->GradientParams::Resize(count);
        }
        const std::size_t limit = std::min(count, this->colors.size());
        for (std::size_t i = 0; i < limit; ++i) {
            this->colors[i] = colors[i];
        }
    }

    /**
     * @brief Set a specific gradient key (index is clamped to valid range).
     */
    void SetSpectrumAt(std::size_t index, const RGBColor& color) {
        if (this->colors.empty()) return;
        if (index >= this->colors.size()) index = this->colors.size() - 1;
        this->colors[index] = color;
    }

    /** @brief Retrieve a gradient key with clamped index. */
    [[nodiscard]] RGBColor GetSpectrumAt(std::size_t index) const {
        if (this->colors.empty()) return RGBColor();
        if (index >= this->colors.size()) index = this->colors.size() - 1;
        return this->colors[index];
    }

    /** @brief Mutable pointer to the gradient array (may be null when empty). */
    RGBColor* SpectrumData() { return this->colors.empty() ? nullptr : this->colors.data(); }

    /** @brief Const pointer to the gradient array (may be null when empty). */
    const RGBColor* SpectrumData() const { return this->colors.empty() ? nullptr : this->colors.data(); }

    /**
     * @brief Define the base palette used for hue shifting/restoration.
     * @param palette Pointer to palette data (non-owning).
     * @param count   Number of entries available at @p palette.
     */
    void SetBasePalette(const RGBColor* palette, std::size_t count) {
        basePalette_      = palette;
        basePaletteCount_ = palette ? count : 0;
        this->baseColors  = palette;
    }

    /**
     * @brief Apply hue shift relative to the stored base palette.
     * @param hueDeg Degrees of hue rotation.
     */
    void HueShift(float hueDeg) {
        if (!basePalette_) return;
        const std::size_t limit = std::min<std::size_t>(basePaletteCount_, this->colors.size());
        for (std::size_t i = 0; i < limit; ++i) {
            RGBColor base = basePalette_[i];
            this->colors[i] = base.HueShift(hueDeg);
        }
    }

    /**
     * @brief Refresh gradient colors from the stored base palette.
     */
    void UpdateRGB() {
        if (!basePalette_) return;
        const std::size_t limit = std::min<std::size_t>(basePaletteCount_, this->colors.size());
        for (std::size_t i = 0; i < limit; ++i) {
            this->colors[i] = basePalette_[i];
        }
    }

private:
    const RGBColor* basePalette_{nullptr};
    std::size_t     basePaletteCount_{0};

    PTX_BEGIN_FIELDS(GradientMaterial)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(GradientMaterial)
        PTX_METHOD_AUTO(GradientMaterial, SetPositionOffset, "Set position offset"),
        PTX_METHOD_AUTO(GradientMaterial, SetRotationOffset, "Set rotation offset"),
        PTX_METHOD_AUTO(GradientMaterial, SetRotationAngle, "Set rotation angle"),
        PTX_METHOD_AUTO(GradientMaterial, SetGradientPeriod, "Set gradient period"),
        PTX_METHOD_AUTO(GradientMaterial, GradientShift, "Gradient shift"),
        PTX_METHOD_AUTO(GradientMaterial, SetColorCount, "Set color count"),
        PTX_METHOD_AUTO(GradientMaterial, ColorCount, "Color count"),
        PTX_METHOD_AUTO(GradientMaterial, UpdateGradient, "Update gradient"),
        PTX_METHOD_AUTO(GradientMaterial, SetSpectrumAt, "Set spectrum at"),
        PTX_METHOD_AUTO(GradientMaterial, GetSpectrumAt, "Get spectrum at"),
        /* Spectrum data */ PTX_METHOD_OVLD0(GradientMaterial, SpectrumData, RGBColor *),
        /* Spectrum data */ PTX_METHOD_OVLD_CONST0(GradientMaterial, SpectrumData, const RGBColor *),
        PTX_METHOD_AUTO(GradientMaterial, SetBasePalette, "Set base palette"),
        PTX_METHOD_AUTO(GradientMaterial, HueShift, "Hue shift"),
        PTX_METHOD_AUTO(GradientMaterial, UpdateRGB, "Update rgb")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(GradientMaterial)
        PTX_CTOR(GradientMaterial, std::size_t, const RGBColor *, float, bool, bool)
    PTX_END_DESCRIBE(GradientMaterial)

};
