// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file colorgray.hpp
 * @brief Grayscale color implementation (8-bit).
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
 * @class ColorGray
 * @brief 8-bit grayscale color.
 *
 * Compact format for monochrome displays or memory-constrained systems.
 * Provides 256 shades of gray.
 */
class ColorGray : public IColor {
public:
    uint8_t value;  ///< Grayscale value (0-255, 0=black, 255=white)

    /**
     * @brief Default constructor (black).
     */
    ColorGray() : value(0) {}

    /**
     * @brief Construct from grayscale value.
     */
    explicit ColorGray(uint8_t gray) : value(gray) {}

    /**
     * @brief Construct from RGB values (converted to grayscale).
     */
    ColorGray(uint8_t r, uint8_t g, uint8_t b) {
        value = RGBToGrayscale(r, g, b);
    }

    /**
     * @brief Copy constructor.
     */
    ColorGray(const ColorGray& other) : value(other.value) {}

    // IColor interface
    ColorFormat GetFormat() const override { return ColorFormat::Grayscale8; }

    void ToRGB888(uint8_t& outR, uint8_t& outG, uint8_t& outB) const override {
        outR = outG = outB = value;
    }

    uint16_t ToRGB565() const override {
        return RGB888ToRGB565(value, value, value);
    }

    void ToRGBA8888(uint8_t& outR, uint8_t& outG, uint8_t& outB, uint8_t& outA) const override {
        outR = outG = outB = value;
        outA = 255;  // Opaque
    }

    uint8_t ToGrayscale8() const override {
        return value;
    }

    bool ToMonochrome() const override {
        return value >= 128;
    }

    void FromRGB888(uint8_t r, uint8_t g, uint8_t b) override {
        value = RGBToGrayscale(r, g, b);
    }

    void FromRGB565(uint16_t rgb565) override {
        uint8_t r, g, b;
        RGB565ToRGB888(rgb565, r, g, b);
        value = RGBToGrayscale(r, g, b);
    }

    void FromGrayscale8(uint8_t gray) override {
        value = gray;
    }

    void FromMonochrome(bool on) override {
        value = on ? 255 : 0;
    }

    IColor* Clone() const override {
        return new ColorGray(*this);
    }

    // Convenience methods
    void Set(uint8_t gray) { value = gray; }
    uint8_t Get() const { return value; }

    /**
     * @brief Scale brightness.
     */
    ColorGray Scale(uint8_t maxBrightness) const {
        return ColorGray(static_cast<uint8_t>((value * maxBrightness) / 255));
    }

    /**
     * @brief Add value.
     */
    ColorGray Add(uint8_t amount) const {
        int result = value + amount;
        return ColorGray(static_cast<uint8_t>(result > 255 ? 255 : result));
    }

    /**
     * @brief Linear interpolation between two grayscale colors.
     */
    static ColorGray Lerp(const ColorGray& a, const ColorGray& b, float t) {
        return ColorGray(static_cast<uint8_t>(a.value + (b.value - a.value) * t));
    }

    /**
     * @brief Convert to string representation.
     */
    UString ToString() const {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Gray(%d)", value);
        return UString(buffer);
    }

    // Operators
    ColorGray operator+(const ColorGray& other) const {
        int result = value + other.value;
        return ColorGray(static_cast<uint8_t>(result > 255 ? 255 : result));
    }

    ColorGray& operator+=(const ColorGray& other) {
        int result = value + other.value;
        value = static_cast<uint8_t>(result > 255 ? 255 : result);
        return *this;
    }

    ColorGray operator-(const ColorGray& other) const {
        int result = value - other.value;
        return ColorGray(static_cast<uint8_t>(result < 0 ? 0 : result));
    }

    ColorGray& operator-=(const ColorGray& other) {
        int result = value - other.value;
        value = static_cast<uint8_t>(result < 0 ? 0 : result);
        return *this;
    }

    ColorGray operator*(float scalar) const {
        int result = static_cast<int>(value * scalar);
        return ColorGray(static_cast<uint8_t>(result > 255 ? 255 : (result < 0 ? 0 : result)));
    }

    ColorGray& operator*=(float scalar) {
        int result = static_cast<int>(value * scalar);
        value = static_cast<uint8_t>(result > 255 ? 255 : (result < 0 ? 0 : result));
        return *this;
    }

    bool operator==(const ColorGray& other) const {
        return value == other.value;
    }

    bool operator!=(const ColorGray& other) const {
        return value != other.value;
    }

    bool operator<(const ColorGray& other) const {
        return value < other.value;
    }

    bool operator>(const ColorGray& other) const {
        return value > other.value;
    }

    // Reflection
    KL_BEGIN_FIELDS(ColorGray)
        KL_FIELD(ColorGray, value, "Value", 0, 255)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ColorGray)
        KL_METHOD_AUTO(ColorGray, Set, "Set"),
        KL_METHOD_AUTO(ColorGray, Get, "Get"),
        KL_METHOD_AUTO(ColorGray, Scale, "Scale"),
        KL_METHOD_AUTO(ColorGray, Add, "Add"),
        KL_SMETHOD_AUTO(ColorGray::Lerp, "Lerp"),
        KL_METHOD_AUTO(ColorGray, ToString, "To string")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ColorGray)
        KL_CTOR0(ColorGray),
        KL_CTOR(ColorGray, uint8_t),
        KL_CTOR(ColorGray, uint8_t, uint8_t, uint8_t),
        KL_CTOR(ColorGray, const ColorGray &)
    KL_END_DESCRIBE(ColorGray)
};

} // namespace koilo
