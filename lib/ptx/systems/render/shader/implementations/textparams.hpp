// textparams.hpp
#pragma once

#include <cstddef>
#include <stdint.h>
#include "../../../../registry/reflect_macros.hpp"

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

    /* NOTE: TextBuilderParamsT is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(TextBuilderParamsT)
        PTX_FIELD(TextBuilderParamsT, size, "Size", 0, 0),
        PTX_FIELD(TextBuilderParamsT, positionOffset, "Position offset", 0, 0),
        PTX_FIELD(TextBuilderParamsT, rotationOffset, "Rotation offset", 0, 0),
        PTX_FIELD(TextBuilderParamsT, rotationAngleDeg, "Rotation angle deg", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(TextBuilderParamsT, isEfficient, "Is efficient", 0, 1),
        PTX_FIELD(TextBuilderParamsT, blinkPeriodMs, "Blink period ms", 0, 4294967295),
        PTX_FIELD(TextBuilderParamsT, elapsedMs, "Elapsed ms", __FLT_MIN__, __FLT_MAX__),
        PTX_FIELD(TextBuilderParamsT, fgWhite, "Fg white", 0, 0),
        PTX_FIELD(TextBuilderParamsT, bgBlack, "Bg black", 0, 0),
        PTX_FIELD(TextBuilderParamsT, fillMat, "Fill mat", 0, 0),
        PTX_FIELD(TextBuilderParamsT, lines, "Lines", 0, 0)
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(TextBuilderParamsT)
        PTX_METHOD_AUTO(TextBuilderParamsT, Clear, "Clear")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(TextBuilderParamsT)
        /* TODO: PTX_CTOR0(TextBuilderParamsT) or PTX_CTOR(TextBuilderParamsT, ...) */
    PTX_END_DESCRIBE(TextBuilderParamsT)
    
};
