// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file colormono.hpp
 * @brief Monochrome color implementation (1-bit on/off).
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include "color.hpp"
#include <koilo/core/platform/ustring.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class ColorMono
 * @brief 1-bit monochrome color (on/off, black/white).
 *
 * Ultra-compact format for simple monochrome displays (e-ink, OLED, LED matrices).
 * Represents only two states: on (white) or off (black).
 */
class ColorMono : public IColor {
public:
    bool on;  ///< true = white/on, false = black/off

    /**
     * @brief Default constructor (off/black).
     */
    ColorMono() : on(false) {}

    /**
     * @brief Construct from boolean value.
     */
    explicit ColorMono(bool state) : on(state) {}

    /**
     * @brief Construct from grayscale value (converted with threshold).
     */
    explicit ColorMono(uint8_t gray, uint8_t threshold = 128) : on(gray >= threshold) {}

    /**
     * @brief Construct from RGB values (converted to monochrome).
     */
    ColorMono(uint8_t r, uint8_t g, uint8_t b, uint8_t threshold = 128) {
        on = RGBToMonochrome(r, g, b, threshold);
    }

    /**
     * @brief Copy constructor.
     */
    ColorMono(const ColorMono& other) : on(other.on) {}

    // IColor interface
    ColorFormat GetFormat() const override { return ColorFormat::Monochrome; }

    void ToRGB888(uint8_t& outR, uint8_t& outG, uint8_t& outB) const override {
        outR = outG = outB = on ? 255 : 0;
    }

    uint16_t ToRGB565() const override {
        return on ? 0xFFFF : 0x0000;
    }

    void ToRGBA8888(uint8_t& outR, uint8_t& outG, uint8_t& outB, uint8_t& outA) const override {
        outR = outG = outB = on ? 255 : 0;
        outA = 255;  // Opaque
    }

    uint8_t ToGrayscale8() const override {
        return on ? 255 : 0;
    }

    bool ToMonochrome() const override {
        return on;
    }

    void FromRGB888(uint8_t r, uint8_t g, uint8_t b) override {
        on = RGBToMonochrome(r, g, b);
    }

    void FromRGB565(uint16_t rgb565) override {
        uint8_t r, g, b;
        RGB565ToRGB888(rgb565, r, g, b);
        on = RGBToMonochrome(r, g, b);
    }

    void FromGrayscale8(uint8_t gray) override {
        on = gray >= 128;
    }

    void FromMonochrome(bool state) override {
        on = state;
    }

    IColor* Clone() const override {
        return new ColorMono(*this);
    }

    // Convenience methods
    void Set(bool state) { on = state; }
    bool Get() const { return on; }
    
    void TurnOn() { on = true; }
    void TurnOff() { on = false; }
    void Toggle() { on = !on; }

    /**
     * @brief Convert to string representation.
     */
    UString ToString() const {
        return UString(on ? "Mono(ON)" : "Mono(OFF)");
    }

    // Operators
    bool operator==(const ColorMono& other) const {
        return on == other.on;
    }

    bool operator!=(const ColorMono& other) const {
        return on != other.on;
    }

    ColorMono operator&&(const ColorMono& other) const {
        return ColorMono(on && other.on);
    }

    ColorMono operator||(const ColorMono& other) const {
        return ColorMono(on || other.on);
    }

    ColorMono operator^(const ColorMono& other) const {
        return ColorMono(on != other.on);
    }

    ColorMono operator!() const {
        return ColorMono(!on);
    }

    // Reflection
    KL_BEGIN_FIELDS(ColorMono)
        KL_FIELD(ColorMono, on, "On", 0, 1)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ColorMono)
        KL_METHOD_AUTO(ColorMono, Set, "Set"),
        KL_METHOD_AUTO(ColorMono, Get, "Get"),
        KL_METHOD_AUTO(ColorMono, TurnOn, "Turn on"),
        KL_METHOD_AUTO(ColorMono, TurnOff, "Turn off"),
        KL_METHOD_AUTO(ColorMono, Toggle, "Toggle"),
        KL_METHOD_AUTO(ColorMono, ToString, "To string")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ColorMono)
        KL_CTOR0(ColorMono),
        KL_CTOR(ColorMono, bool),
        KL_CTOR(ColorMono, uint8_t, uint8_t),
        KL_CTOR(ColorMono, uint8_t, uint8_t, uint8_t, uint8_t),
        KL_CTOR(ColorMono, const ColorMono &)
    KL_END_DESCRIBE(ColorMono)
};

} // namespace koilo
