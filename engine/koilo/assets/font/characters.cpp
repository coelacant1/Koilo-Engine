// SPDX-License-Identifier: GPL-3.0-or-later
// characters.cpp
#include <koilo/assets/font/characters.hpp>


namespace koilo {

/** 
 * @file characters.cpp
 * @brief Bitmap data and lookup implementation for 8x8 character glyphs.
 * @date 8/18/2025
 * @author Coela Can't
 *
 * Each glyph is an array of 8 bytes. Bit 7 (MSB) is the left-most pixel. Rows are ordered
 * top-to-bottom. These tables are intended for low-level raster displays (LED matrices, etc.).
 */
/**
 * @copydoc koilo::Characters::GetCharacter(char)
 */
const uint8_t* koilo::Characters::GetCharacter(char character){
    switch (character){
        case ':': return COLON;
        case '/': return FWDSLSH;
        case '.': return PERIOD;
        case ',': return COMMA;
        case '!': return EXCLAIM;
        case '?': return QUESTION;
        case '-': return DASH;
        case '+': return PLUS;
        case '=': return EQUALS;
        case '(': return LPAREN;
        case ')': return RPAREN;
        case '<': return LANGLE;
        case '>': return RANGLE;
        case ';': return SEMICOLON;
        case '_': return UNDERSCORE;
        case '#': return HASH;
        case '@': return AT;
        case '%': return PERCENT;
        case '*': return ASTERISK;
        case '&': return AMPERSAND;
        case '\'': return QUOTE;
        case '"': return DQUOTE;
        case '[': return LBRACKET;
        case ']': return RBRACKET;
        case '0': return N0;
        case '1': return N1;
        case '2': return N2;
        case '3': return N3;
        case '4': return N4;
        case '5': return N5;
        case '6': return N6;
        case '7': return N7;
        case '8': return N8;
        case '9': return N9;
        case 'a': case 'A': return LA;
        case 'b': case 'B': return LB;
        case 'c': case 'C': return LC;
        case 'd': case 'D': return LD;
        case 'e': case 'E': return LE;
        case 'f': case 'F': return LF;
        case 'g': case 'G': return LG;
        case 'h': case 'H': return LH;
        case 'i': case 'I': return LI;
        case 'j': case 'J': return LJ;
        case 'k': case 'K': return LK;
        case 'l': case 'L': return LL;
        case 'm': case 'M': return LM;
        case 'n': case 'N': return LN;
        case 'o': case 'O': return LO;
        case 'p': case 'P': return LP;
        case 'q': case 'Q': return LQ;
        case 'r': case 'R': return LR;
        case 's': case 'S': return LS;
        case 't': case 'T': return LT;
        case 'u': case 'U': return LU;
        case 'v': case 'V': return LV;
        case 'w': case 'W': return LW;
        case 'x': case 'X': return LX;
        case 'y': case 'Y': return LY;
        case 'z': case 'Z': return LZ;
        default: return SPACE;
    }
}

const uint8_t koilo::Characters::COLON[8] = { 
    0b00000000,
    0b00000000,
    0b00111000,
    0b00111000,
    0b00000000,
    0b00000000,
    0b00111000,
    0b00111000
};

const uint8_t koilo::Characters::FWDSLSH[8] = { 
    0b00000001,
    0b00000011,
    0b00000110,
    0b00001100,
    0b00011000,
    0b00110000,
    0b01100000,
    0b11000000
};

const uint8_t koilo::Characters::SPACE[8] = { 
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
};

const uint8_t koilo::Characters::N0[8] = { 
    0b01111110,
    0b11111111,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11111111,
    0b01111110
};

const uint8_t koilo::Characters::N1[8] = { 
    0b00011000,
    0b01111000,
    0b11011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b11111111
};

const uint8_t koilo::Characters::N2[8] = { 
    0b00111100,
    0b01111110,
    0b11000011,
    0b00000011,
    0b00000110,
    0b00011000,
    0b01100000,
    0b11111111
};

const uint8_t koilo::Characters::N3[8] = { 
    0b01111110,
    0b11000011,
    0b00001100,
    0b00111000,
    0b00001100,
    0b11000011,
    0b01100110,
    0b00111100
};

const uint8_t koilo::Characters::N4[8] = { 
    0b00000110,
    0b00001110,
    0b00011110,
    0b00110110,
    0b01100110,
    0b11111111,
    0b00001100,
    0b00001100
};

const uint8_t koilo::Characters::N5[8] = { 
    0b11111111,
    0b11000000,
    0b11000000,
    0b11111100,
    0b00000110,
    0b11000011,
    0b01100110,
    0b00111100
};

const uint8_t koilo::Characters::N6[8] = { 
    0b00111111,
    0b01100000,
    0b11000000,
    0b11000000,
    0b11111110,
    0b11000011,
    0b11000011,
    0b01111110
};

const uint8_t koilo::Characters::N7[8] = { 
    0b11111111,
    0b00000011,
    0b00000110,
    0b00001100,
    0b00011000,
    0b00110000,
    0b01100000,
    0b11000000
};

const uint8_t koilo::Characters::N8[8] = { 
    0b01111110,
    0b11000011,
    0b11000011,
    0b01111110,
    0b01111110,
    0b11000011,
    0b11000011,
    0b01111110
};

const uint8_t koilo::Characters::N9[8] = { 
    0b01111110,
    0b11000011,
    0b11000011,
    0b01111111,
    0b00000110,
    0b00001100,
    0b00011000,
    0b00110000
};


const uint8_t koilo::Characters::LA[8] = { 
    0b00111100,
    0b01100110,
    0b11000011,
    0b11000011,
    0b11111111,
    0b11000011,
    0b11000011,
    0b11000011
};

const uint8_t koilo::Characters::LB[8] = { 
    0b11111110,
    0b11000011,
    0b11000011,
    0b11111100,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11111110
};

const uint8_t koilo::Characters::LC[8] = { 
    0b01111110,
    0b11000011,
    0b11000000,
    0b11000000,
    0b11000000,
    0b11000000,
    0b11000011,
    0b01111110
};

const uint8_t koilo::Characters::LD[8] = { 
    0b11111100,
    0b11000110,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11000110,
    0b11111100
};

const uint8_t koilo::Characters::LE[8] = { 
    0b11111111,
    0b11000000,
    0b11000000,
    0b11111111,
    0b11000000,
    0b11000000,
    0b11000000,
    0b11111111
};

const uint8_t koilo::Characters::LF[8] = { 
    0b11111111,
    0b11000000,
    0b11000000,
    0b11111111,
    0b11000000,
    0b11000000,
    0b11000000,
    0b11000000
};

const uint8_t koilo::Characters::LG[8] = { 
    0b01111110,
    0b11000011,
    0b11000000,
    0b11000000,
    0b11000110,
    0b11000011,
    0b11000011,
    0b01111110
};

const uint8_t koilo::Characters::LH[8] = { 
    0b11000011,
    0b11000011,
    0b11000011,
    0b11111111,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11000011
};

const uint8_t koilo::Characters::LI[8] = { 
    0b11111111,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b11111111
};

const uint8_t koilo::Characters::LJ[8] = { 
    0b11111111,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b11011000,
    0b01110000
};

const uint8_t koilo::Characters::LK[8] = { 
    0b11000110,
    0b11001100,
    0b11011000,
    0b11110000,
    0b11011000,
    0b11001100,
    0b11000110,
    0b11000011
};

const uint8_t koilo::Characters::LL[8] = { 
    0b11000000,
    0b11000000,
    0b11000000,
    0b11000000,
    0b11000000,
    0b11000000,
    0b11000000,
    0b11111111
};

const uint8_t koilo::Characters::LM[8] = { 
    0b11100111,
    0b11111111,
    0b11011011,
    0b11011011,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11000011
};

const uint8_t koilo::Characters::LN[8] = { 
    0b11000011,
    0b11000011,
    0b11100011,
    0b11110011,
    0b11011011,
    0b11001111,
    0b11000111,
    0b11000011
};

const uint8_t koilo::Characters::LO[8] = { 
    0b01111110,
    0b11111111,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11111111,
    0b01111110
};

const uint8_t koilo::Characters::LP[8] = { 
    0b11111110,
    0b11000011,
    0b11000011,
    0b11111110,
    0b11000000,
    0b11000000,
    0b11000000,
    0b11000000
};

const uint8_t koilo::Characters::LQ[8] = { 
    0b01111110,
    0b11111111,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11001110,
    0b11111010,
    0b01111001
};

const uint8_t koilo::Characters::LR[8] = { 
    0b11111110,
    0b11000011,
    0b11000011,
    0b11111110,
    0b11011000,
    0b11001100,
    0b11000110,
    0b11000011
};

const uint8_t koilo::Characters::LS[8] = { 
    0b01111111,
    0b11000000,
    0b11000000,
    0b01111110,
    0b00000011,
    0b00000011,
    0b00000011,
    0b11111110
};

const uint8_t koilo::Characters::LT[8] = { 
    0b11111111,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000
};

const uint8_t koilo::Characters::LU[8] = { 
    0b11000011,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11111111,
    0b01111110
};

const uint8_t koilo::Characters::LV[8] = { 
    0b11000011,
    0b11000011,
    0b11000011,
    0b01100110,
    0b01100110,
    0b01100110,
    0b00111100,
    0b00111100
};

const uint8_t koilo::Characters::LW[8] = { 
    0b11000011,
    0b11000011,
    0b11000011,
    0b11000011,
    0b11011011,
    0b11111111,
    0b11100111,
    0b11000011,
};

const uint8_t koilo::Characters::LX[8] = { 
    0b11000011,
    0b01100110,
    0b00111100,
    0b00011000,
    0b00011000,
    0b00111100,
    0b01100110,
    0b11000011
};

const uint8_t koilo::Characters::LY[8] = { 
    0b11000011,
    0b01100110,
    0b00111100,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000
};

const uint8_t koilo::Characters::LZ[8] = { 
    0b11111111,
    0b00000110,
    0b00001100,
    0b00011000,
    0b00110000,
    0b01100000,
    0b11000000,
    0b11111111,
};

// -------- Additional punctuation glyphs --------

const uint8_t koilo::Characters::PERIOD[8] = {
    0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00110000, 0b00110000
};

const uint8_t koilo::Characters::COMMA[8] = {
    0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b00110000, 0b00110000, 0b01100000
};

const uint8_t koilo::Characters::EXCLAIM[8] = {
    0b00011000, 0b00011000, 0b00011000, 0b00011000,
    0b00011000, 0b00000000, 0b00011000, 0b00011000
};

const uint8_t koilo::Characters::QUESTION[8] = {
    0b00111100, 0b01100110, 0b00000110, 0b00001100,
    0b00011000, 0b00000000, 0b00011000, 0b00011000
};

const uint8_t koilo::Characters::DASH[8] = {
    0b00000000, 0b00000000, 0b00000000, 0b01111110,
    0b01111110, 0b00000000, 0b00000000, 0b00000000
};

const uint8_t koilo::Characters::PLUS[8] = {
    0b00000000, 0b00011000, 0b00011000, 0b01111110,
    0b01111110, 0b00011000, 0b00011000, 0b00000000
};

const uint8_t koilo::Characters::EQUALS[8] = {
    0b00000000, 0b00000000, 0b01111110, 0b00000000,
    0b00000000, 0b01111110, 0b00000000, 0b00000000
};

const uint8_t koilo::Characters::LPAREN[8] = {
    0b00001100, 0b00011000, 0b00110000, 0b00110000,
    0b00110000, 0b00110000, 0b00011000, 0b00001100
};

const uint8_t koilo::Characters::RPAREN[8] = {
    0b00110000, 0b00011000, 0b00001100, 0b00001100,
    0b00001100, 0b00001100, 0b00011000, 0b00110000
};

const uint8_t koilo::Characters::LANGLE[8] = {
    0b00000110, 0b00001100, 0b00011000, 0b00110000,
    0b00110000, 0b00011000, 0b00001100, 0b00000110
};

const uint8_t koilo::Characters::RANGLE[8] = {
    0b01100000, 0b00110000, 0b00011000, 0b00001100,
    0b00001100, 0b00011000, 0b00110000, 0b01100000
};

const uint8_t koilo::Characters::SEMICOLON[8] = {
    0b00000000, 0b00000000, 0b00110000, 0b00110000,
    0b00000000, 0b00110000, 0b00110000, 0b01100000
};

const uint8_t koilo::Characters::UNDERSCORE[8] = {
    0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b11111111
};

const uint8_t koilo::Characters::HASH[8] = {
    0b00100100, 0b00100100, 0b11111111, 0b00100100,
    0b00100100, 0b11111111, 0b00100100, 0b00100100
};

const uint8_t koilo::Characters::AT[8] = {
    0b00111100, 0b01000010, 0b01011110, 0b01010010,
    0b01011110, 0b01000000, 0b00111110, 0b00000000
};

const uint8_t koilo::Characters::PERCENT[8] = {
    0b01100010, 0b01100100, 0b00001000, 0b00010000,
    0b00100000, 0b01001100, 0b10001100, 0b00000000
};

const uint8_t koilo::Characters::ASTERISK[8] = {
    0b00000000, 0b00100100, 0b00011000, 0b01111110,
    0b00011000, 0b00100100, 0b00000000, 0b00000000
};

const uint8_t koilo::Characters::AMPERSAND[8] = {
    0b00111000, 0b01001000, 0b00110000, 0b01001000,
    0b10010100, 0b10001000, 0b01110110, 0b00000000
};

const uint8_t koilo::Characters::QUOTE[8] = {
    0b00011000, 0b00011000, 0b00010000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000
};

const uint8_t koilo::Characters::DQUOTE[8] = {
    0b01100110, 0b01100110, 0b01000100, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000
};

const uint8_t koilo::Characters::LBRACKET[8] = {
    0b00111100, 0b00110000, 0b00110000, 0b00110000,
    0b00110000, 0b00110000, 0b00110000, 0b00111100
};

const uint8_t koilo::Characters::RBRACKET[8] = {
    0b00111100, 0b00001100, 0b00001100, 0b00001100,
    0b00001100, 0b00001100, 0b00001100, 0b00111100
};

} // namespace koilo