#pragma once

#include <cstddef>
#include <cstdint>

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/textparams.hpp"
#include "../../shader/implementations/textshader.hpp"

#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/platform/time.hpp"   // if you want, but we’ll just use Update(dt)

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
 * @brief Text builder material that wraps params, exposes setters, and accumulates time for a blink effect.
 * @tparam lineCount Number of text lines stored in the parameter block.
 * @tparam charWidth Maximum characters per line (fixed-width).
 *
 * @class TextBuilderMaterialT
 * @extends MaterialT<TextBuilderParamsT<lineCount, charWidth>, TextBuilderShaderT<lineCount, charWidth>>
 */
template <uint8_t lineCount, uint8_t charWidth>
class TextBuilderMaterialT
  : public MaterialT<TextBuilderParamsT<lineCount, charWidth>,
                     TextBuilderShaderT<lineCount, charWidth>> {

    using Base = MaterialT<TextBuilderParamsT<lineCount, charWidth>,
                           TextBuilderShaderT<lineCount, charWidth>>;

public:
    /**
     * @brief Construct and optionally enable the efficient rendering mode.
     * @param efficient When true, sets the internal @c isEfficient flag.
     */
    TextBuilderMaterialT(bool efficient = false) : Base() {
        this->isEfficient = efficient;
        this->Clear();
    }

    // --- Configuration setters ---

    /** @brief Set overall size (logical units). */
    void SetSize(Vector2D s)                    { this->size = s; }

    /** @brief Set position offset. */
    void SetPositionOffset(Vector2D p)          { this->positionOffset = p; }

    /** @brief Set rotation offset (pivot). */
    void SetRotationOffset(Vector2D r)          { this->rotationOffset = r; }

    /** @brief Set rotation angle in degrees. */
    void SetRotationAngle(float deg)            { this->rotationAngleDeg = deg; }

    /** @brief Set blink period in milliseconds. */
    void SetBlinkPeriodMs(uint32_t ms)          { this->blinkPeriodMs = ms; }

    /** @brief Toggle efficient rendering path. */
    void SetEfficient(bool e)                   { this->isEfficient = e; }

    /** @brief Set foreground (text) color. */
    void SetForegroundColor(const RGBColor& c)  { this->fgWhite = c; }

    /** @brief Set background color. */
    void SetBackgroundColor(const RGBColor& c)  { this->bgBlack = c; }

    /** @brief Set optional fill material used by the shader. */
    void SetFillMaterial(const IMaterial* m)    { this->fillMat = m; }

    // Text API

    /** @brief Clear all lines to spaces (via parameter block). */
    void Clear()                                { this->TextParams().Clear(); }

    /**
     * @brief Set text of a specific line from a buffer with length.
     * @param line       Zero-based line index; ignored if @p line >= lineCount.
     * @param data       Pointer to character data (not necessarily null-terminated).
     * @param len        Number of characters to copy (clamped to @c charWidth).
     * @param centerText When true, horizontally centers the text within the line if it fits.
     *
     * Fills the entire line with spaces before writing the substring. If @p centerText is true
     * and @p len exceeds @c charWidth, the call returns early with no change.
     */
    void SetText(uint8_t line, const char* data, uint8_t len, bool centerText = false) {
        if (line >= lineCount) return;

        int start = 0;
        if (centerText) {
            int spacing = (int(charWidth) - int(len)) / 2;
            if (spacing < 0) return;
            start = spacing;
        }
        if (len > charWidth) len = charWidth;

        // fill spaces first
        for (uint8_t c = 0; c < charWidth; ++c) this->lines[line][c] = ' ';

        for (uint8_t i = 0; i < len; ++i) {
            this->lines[line][start + i] = data[i];
        }
    }

    /**
     * @brief Set text of a specific line from a null-terminated C string.
     * @param line       Zero-based line index.
     * @param cstr       Null-terminated string.
     * @param centerText When true, horizontally centers the text within the line if it fits.
     */
    void SetText(uint8_t line, const char* cstr, bool centerText = false) {
        uint8_t len = 0;
        while (cstr[len] != '\0' && len < 0xFE) ++len; // safe upper bound
        SetText(line, cstr, len, centerText);
    }

    /**
     * @brief Per-frame update; accumulate elapsed time for blinking effects.
     * @param dt Delta time in seconds (expected); converted to milliseconds internally.
     */
    void Update(float dt) override {
        // dt expected in seconds; convert to ms
        this->elapsedMs += dt * 1000.0f;
    }

private:
    /** @brief Shorthand to access the parameter block. */
    TextBuilderParamsT<lineCount, charWidth>& TextParams() { return *this; }

};
