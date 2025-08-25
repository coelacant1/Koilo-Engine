// characters.hpp
#pragma once

#include <stdint.h>

/**
 * @file characters.hpp
 * @brief 8x8 bitmap glyph table and lookup for ASCII-like characters.
 * @date 8/18/2025
 * @author Coela Can't
 *
 * This header declares an immutable bitmap font comprised of 8x8 glyphs for a subset
 * of ASCII characters. Each glyph is encoded as 8 bytes (top-to-bottom), where each
 * bit (MSB -> leftmost pixel) represents a lit pixel (1 = on, 0 = off).
 *
 * Use Characters::GetCharacter() to retrieve a pointer to the 8-byte bitmap for a given
 * character. Unknown characters return the SPACE glyph.
 */
class Characters {
public:
    // -------- Punctuation / spacing --------
    static const uint8_t COLON[8];     ///< ':' glyph (8 rows x 8 bits).
    static const uint8_t FWDSLSH[8];   ///< '/' glyph.
    static const uint8_t SPACE[8];     ///< ' ' space glyph (blank).

    // -------- Digits --------
    static const uint8_t N0[8]; ///< '0'
    static const uint8_t N1[8]; ///< '1'
    static const uint8_t N2[8]; ///< '2'
    static const uint8_t N3[8]; ///< '3'
    static const uint8_t N4[8]; ///< '4'
    static const uint8_t N5[8]; ///< '5'
    static const uint8_t N6[8]; ///< '6'
    static const uint8_t N7[8]; ///< '7'
    static const uint8_t N8[8]; ///< '8'
    static const uint8_t N9[8]; ///< '9'

    // -------- Letters --------
    static const uint8_t LA[8];
    static const uint8_t LB[8];
    static const uint8_t LC[8];
    static const uint8_t LD[8];
    static const uint8_t LE[8];
    static const uint8_t LF[8];
    static const uint8_t LG[8];
    static const uint8_t LH[8];
    static const uint8_t LI[8];
    static const uint8_t LJ[8];
    static const uint8_t LK[8];
    static const uint8_t LL[8];
    static const uint8_t LM[8];
    static const uint8_t LN[8];
    static const uint8_t LO[8];
    static const uint8_t LP[8];
    static const uint8_t LQ[8];
    static const uint8_t LR[8];
    static const uint8_t LS[8];
    static const uint8_t LT[8];
    static const uint8_t LU[8];
    static const uint8_t LV[8];
    static const uint8_t LW[8];
    static const uint8_t LX[8];
    static const uint8_t LY[8];
    static const uint8_t LZ[8];

    /**
     * @brief Returns the 8x8 bitmap for a character.
     * @param character ASCII character to look up (digits, letters, and select punctuation).
     * @return Pointer to an 8-byte array (rows top->bottom), or SPACE if unsupported.
     * @note The returned pointer refers to a static internal table; do not free or modify it.
     */
    static const uint8_t* GetCharacter(char character);
};
