// textparams.hpp
#pragma once

#include <cstddef>
#include <stdint.h>

#include "../../../../assets/font/characters.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/math/vector3d.hpp"

class IMaterial; // fwd

/**
 * @file textparams.hpp
 * @brief Parameter block for a fixed-line, fixed-width text builder.
 *
 * @tparam lineCount Number of text rows.
 * @tparam charWidth Number of characters per row.
 *
 * Holds layout (size/offset/rotation), blink timing, foreground/background colors,
 * optional fill sub-material, and a space-initialized text buffer.
 */
template <uint8_t lineCount, uint8_t charWidth>
struct TextBuilderParamsT {
    // --- Layout ---
    Vector2D size             {192.0f, 96.0f};  ///< Total pixel area of the text canvas.
    Vector2D positionOffset   {0.0f,  0.0f};    ///< Canvas origin offset.
    Vector2D rotationOffset   {0.0f,  0.0f};    ///< Rotation pivot in canvas space.
    float    rotationAngleDeg {0.0f};           ///< Rotation angle (degrees).
    bool     isEfficient      {false};          ///< Enable 90°/180°/270° fast-path.

    // --- Blink ---
    uint32_t blinkPeriodMs    {500};            ///< One on/off phase length (ms).
    float    elapsedMs        {0.0f};           ///< Elapsed time (advance externally).

    // --- Colors ---
    RGBColor fgWhite          {255,255,255};    ///< Foreground (glyph) color.
    RGBColor bgBlack          {  0,  0,  0};    ///< Background color.

    // --- Optional nested material for glyph fill (non-owning) ---
    const IMaterial* fillMat  {nullptr};

    // --- Text buffer (space-initialized) ---
    char lines[lineCount][charWidth];

    /**
     * @brief Clear all characters to space.
     */
    void Clear() {
        for (uint8_t r = 0; r < lineCount; ++r)
            for (uint8_t c = 0; c < charWidth; ++c)
                lines[r][c] = ' ';
    }
};
