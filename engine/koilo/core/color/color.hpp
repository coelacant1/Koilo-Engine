// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file color.hpp
 * @brief Base color interface for generalized color representation.
 *
 * Provides a unified interface for different color formats (RGB888, RGB565, 
 * Grayscale, Monochrome, etc.) with efficient conversion capabilities.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <cstdint>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @enum ColorFormat
 * @brief Enumeration of supported color formats.
 */
enum class ColorFormat : uint8_t {
    RGB888,      ///< 24-bit RGB (8 bits per channel)
    RGB565,      ///< 16-bit RGB (5-6-5 bits)
    RGBA8888,    ///< 32-bit RGBA (8 bits per channel)
    Grayscale8,  ///< 8-bit grayscale
    Monochrome,  ///< 1-bit monochrome (on/off)
    BGR888,      ///< 24-bit BGR (reversed RGB)
    BGRA8888,    ///< 32-bit BGRA (reversed RGBA)
};

/**
 * @class IColor
 * @brief Interface for color objects with format conversion capabilities.
 *
 * This interface provides a common API for color objects of different formats,
 * enabling efficient conversion between formats for rendering to various display types.
 */
class IColor {
public:
    virtual ~IColor() = default;

    /**
     * @brief Get the format of this color.
     */
    virtual ColorFormat GetFormat() const = 0;

    /**
     * @brief Convert to RGB888 format.
     * @param outR Output red component (0-255)
     * @param outG Output green component (0-255)
     * @param outB Output blue component (0-255)
     */
    virtual void ToRGB888(uint8_t& outR, uint8_t& outG, uint8_t& outB) const = 0;

    /**
     * @brief Convert to RGB565 format (packed 16-bit).
     * @return 16-bit packed color value (5 bits R, 6 bits G, 5 bits B)
     */
    virtual uint16_t ToRGB565() const = 0;

    /**
     * @brief Convert to RGBA8888 format.
     * @param outR Output red component (0-255)
     * @param outG Output green component (0-255)
     * @param outB Output blue component (0-255)
     * @param outA Output alpha component (0-255)
     */
    virtual void ToRGBA8888(uint8_t& outR, uint8_t& outG, uint8_t& outB, uint8_t& outA) const = 0;

    /**
     * @brief Convert to 8-bit grayscale.
     * @return Grayscale value (0-255)
     */
    virtual uint8_t ToGrayscale8() const = 0;

    /**
     * @brief Convert to monochrome (on/off).
     * @return true for "on" (white), false for "off" (black)
     */
    virtual bool ToMonochrome() const = 0;

    /**
     * @brief Set color from RGB888 values.
     */
    virtual void FromRGB888(uint8_t r, uint8_t g, uint8_t b) = 0;

    /**
     * @brief Set color from RGB565 packed value.
     */
    virtual void FromRGB565(uint16_t rgb565) = 0;

    /**
     * @brief Set color from grayscale value.
     */
    virtual void FromGrayscale8(uint8_t gray) = 0;

    /**
     * @brief Set color from monochrome value.
     */
    virtual void FromMonochrome(bool on) = 0;

    /**
     * @brief Clone this color object.
     */
    virtual IColor* Clone() const = 0;
};

/**
 * @brief Helper to convert RGB888 to RGB565.
 */
inline uint16_t RGB888ToRGB565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

/**
 * @brief Helper to convert RGB565 to RGB888.
 */
inline void RGB565ToRGB888(uint16_t rgb565, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = ((rgb565 >> 11) & 0x1F) << 3;
    g = ((rgb565 >> 5) & 0x3F) << 2;
    b = (rgb565 & 0x1F) << 3;
}

/**
 * @brief Helper to convert RGB to grayscale using standard luminance formula.
 */
inline uint8_t RGBToGrayscale(uint8_t r, uint8_t g, uint8_t b) {
    // Standard ITU-R BT.601 luma coefficients
    return static_cast<uint8_t>((r * 299 + g * 587 + b * 114) / 1000);
}

/**
 * @brief Helper to convert grayscale to RGB (all channels equal).
 */
inline void GrayscaleToRGB(uint8_t gray, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = g = b = gray;
}

/**
 * @brief Helper to convert RGB to monochrome using threshold.
 */
inline bool RGBToMonochrome(uint8_t r, uint8_t g, uint8_t b, uint8_t threshold = 128) {
    uint8_t gray = RGBToGrayscale(r, g, b);
    return gray >= threshold;
}

/**
 * @brief Helper to convert monochrome to RGB.
 */
inline void MonochromeToRGB(bool on, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = g = b = on ? 255 : 0;
}

} // namespace koilo
