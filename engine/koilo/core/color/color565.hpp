// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file color565.hpp
 * @brief RGB565 color implementation (16-bit color).
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include "color.hpp"
#include <koilo/core/platform/ustring.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <cstdio>

namespace koilo {

/**
 * @class Color565
 * @brief 16-bit RGB color (5-6-5 bits).
 *
 * Compact color format commonly used in embedded displays and microcontrollers.
 * Provides 65,536 colors with reduced memory footprint.
 */
class Color565 : public IColor {
private:
    uint16_t value;  ///< Packed RGB565 value (5R, 6G, 5B)

public:
    /**
     * @brief Default constructor (black).
     */
    Color565() : value(0) {}

    /**
     * @brief Construct from packed RGB565 value.
     */
    explicit Color565(uint16_t packed) : value(packed) {}

    /**
     * @brief Construct from RGB888 values.
     */
    Color565(uint8_t r, uint8_t g, uint8_t b) {
        value = RGB888ToRGB565(r, g, b);
    }

    /**
     * @brief Copy constructor.
     */
    Color565(const Color565& other) : value(other.value) {}

    // IColor interface
    ColorFormat GetFormat() const override { return ColorFormat::RGB565; }

    void ToRGB888(uint8_t& outR, uint8_t& outG, uint8_t& outB) const override {
        RGB565ToRGB888(value, outR, outG, outB);
    }

    uint16_t ToRGB565() const override {
        return value;
    }

    void ToRGBA8888(uint8_t& outR, uint8_t& outG, uint8_t& outB, uint8_t& outA) const override {
        RGB565ToRGB888(value, outR, outG, outB);
        outA = 255;  // Opaque
    }

    uint8_t ToGrayscale8() const override {
        uint8_t r, g, b;
        RGB565ToRGB888(value, r, g, b);
        return RGBToGrayscale(r, g, b);
    }

    bool ToMonochrome() const override {
        uint8_t r, g, b;
        RGB565ToRGB888(value, r, g, b);
        return RGBToMonochrome(r, g, b);
    }

    void FromRGB888(uint8_t r, uint8_t g, uint8_t b) override {
        value = RGB888ToRGB565(r, g, b);
    }

    void FromRGB565(uint16_t rgb565) override {
        value = rgb565;
    }

    void FromGrayscale8(uint8_t gray) override {
        value = RGB888ToRGB565(gray, gray, gray);
    }

    void FromMonochrome(bool on) override {
        value = on ? 0xFFFF : 0x0000;
    }

    IColor* Clone() const override {
        return new Color565(*this);
    }

    // Convenience methods
    void Set(uint16_t packed) { value = packed; }
    void Set(uint8_t r, uint8_t g, uint8_t b) { value = RGB888ToRGB565(r, g, b); }
    uint16_t GetPacked() const { return value; }

    /**
     * @brief Extract red component (0-31 range).
     */
    uint8_t GetR5() const { return (value >> 11) & 0x1F; }

    /**
     * @brief Extract green component (0-63 range).
     */
    uint8_t GetG6() const { return (value >> 5) & 0x3F; }

    /**
     * @brief Extract blue component (0-31 range).
     */
    uint8_t GetB5() const { return value & 0x1F; }

    /**
     * @brief Extract red component scaled to 0-255.
     */
    uint8_t GetR8() const { return GetR5() << 3; }

    /**
     * @brief Extract green component scaled to 0-255.
     */
    uint8_t GetG8() const { return GetG6() << 2; }

    /**
     * @brief Extract blue component scaled to 0-255.
     */
    uint8_t GetB8() const { return GetB5() << 3; }

    /**
     * @brief Linear interpolation between two colors.
     */
    static Color565 Lerp(const Color565& a, const Color565& b, float t) {
        uint8_t r1, g1, b1, r2, g2, b2;
        a.ToRGB888(r1, g1, b1);
        b.ToRGB888(r2, g2, b2);
        
        uint8_t r = static_cast<uint8_t>(r1 + (r2 - r1) * t);
        uint8_t g = static_cast<uint8_t>(g1 + (g2 - g1) * t);
        uint8_t b_ = static_cast<uint8_t>(b1 + (b2 - b1) * t);
        
        return Color565(r, g, b_);
    }

    /**
     * @brief Convert to string representation.
     */
    UString ToString() const {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "RGB565(0x%04X)", value);
        return UString(buffer);
    }

    // Operators
    bool operator==(const Color565& other) const {
        return value == other.value;
    }

    bool operator!=(const Color565& other) const {
        return value != other.value;
    }

    // Reflection
    KL_BEGIN_FIELDS(Color565)
        KL_FIELD(Color565, value, "Value", 0, 65535)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Color565)
        KL_METHOD_OVLD(Color565, Set, void, uint16_t),
        KL_METHOD_OVLD(Color565, Set, void, uint8_t, uint8_t, uint8_t),
        KL_METHOD_AUTO(Color565, GetPacked, "Get packed"),
        KL_METHOD_AUTO(Color565, GetR5, "Get R5"),
        KL_METHOD_AUTO(Color565, GetG6, "Get G6"),
        KL_METHOD_AUTO(Color565, GetB5, "Get B5"),
        KL_METHOD_AUTO(Color565, GetR8, "Get R8"),
        KL_METHOD_AUTO(Color565, GetG8, "Get G8"),
        KL_METHOD_AUTO(Color565, GetB8, "Get B8"),
        KL_SMETHOD_AUTO(Color565::Lerp, "Lerp"),
        KL_METHOD_AUTO(Color565, ToString, "To string")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Color565)
        KL_CTOR0(Color565),
        KL_CTOR(Color565, uint16_t),
        KL_CTOR(Color565, uint8_t, uint8_t, uint8_t),
        KL_CTOR(Color565, const Color565 &)
    KL_END_DESCRIBE(Color565)
};

} // namespace koilo
