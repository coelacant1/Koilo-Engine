// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file color888.hpp
 * @brief RGB888 color implementation (24-bit color).
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <koilo/core/platform/ustring.hpp>
#include <koilo/core/math/mathematics.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/registry/reflect_macros.hpp>
#include <cstdint>
#include <cstdio>

namespace koilo {

/**
 * @class Color888
 * @brief 24-bit RGB color (8 bits per channel), packed POD type.
 *
 * This is the standard full-color format, providing 16.7 million colors.
 * Kept as a plain struct (no vtable) for cache-friendly pixel buffers.
 */
class Color888 {
public:
    union {
        struct {
            uint8_t r;  ///< Red component (0-255)
            uint8_t g;  ///< Green component (0-255)
            uint8_t b;  ///< Blue component (0-255)
        };
        struct {
            uint8_t R;  ///< Red component alias (legacy compatibility)
            uint8_t G;  ///< Green component alias (legacy compatibility)
            uint8_t B;  ///< Blue component alias (legacy compatibility)
        };
    };

    Color888() : r(0), g(0), b(0) {}
    Color888(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
    Color888(const Color888& other) = default;
    Color888& operator=(const Color888&) = default;

    explicit Color888(const Vector3D& vec) 
        : r(static_cast<uint8_t>(Mathematics::Constrain(vec.X, 0.0f, 1.0f) * 255.0f))
        , g(static_cast<uint8_t>(Mathematics::Constrain(vec.Y, 0.0f, 1.0f) * 255.0f))
        , b(static_cast<uint8_t>(Mathematics::Constrain(vec.Z, 0.0f, 1.0f) * 255.0f)) {}

    // Format conversion (previously virtual, now inline)
    void ToRGB888(uint8_t& outR, uint8_t& outG, uint8_t& outB) const {
        outR = r; outG = g; outB = b;
    }

    uint16_t ToRGB565() const {
        return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    }

    void ToRGBA8888(uint8_t& outR, uint8_t& outG, uint8_t& outB, uint8_t& outA) const {
        outR = r; outG = g; outB = b; outA = 255;
    }

    uint8_t ToGrayscale8() const {
        return static_cast<uint8_t>((r * 77 + g * 150 + b * 29) >> 8);
    }

    bool ToMonochrome() const {
        return ToGrayscale8() >= 128;
    }

    void FromRGB888(uint8_t r_, uint8_t g_, uint8_t b_) {
        r = r_; g = g_; b = b_;
    }

    // Convenience methods
    void Set(uint8_t r_, uint8_t g_, uint8_t b_) {
        r = r_; g = g_; b = b_;
    }

    void SetColor(uint8_t r_, uint8_t g_, uint8_t b_) { Set(r_, g_, b_); }

    /**
     * @brief Scale brightness.
     */
    Color888 Scale(uint8_t maxBrightness) const {
        float scale = maxBrightness / 255.0f;
        return Color888(
            static_cast<uint8_t>(r * scale),
            static_cast<uint8_t>(g * scale),
            static_cast<uint8_t>(b * scale)
        );
    }

    /**
     * @brief Add value to all channels.
     */
    Color888 Add(uint8_t value) const {
        return Color888(
            Mathematics::Min(r + value, 255),
            Mathematics::Min(g + value, 255),
            Mathematics::Min(b + value, 255)
        );
    }

    /**
     * @brief Hue shift by angle in degrees.
     */
    Color888 HueShift(float hueDeg) const;

    static Color888 InterpolateColors(const Color888& a, const Color888& b, float ratio) {
        Color888 result;
        result.r = static_cast<uint8_t>(Mathematics::Constrain(a.r + (b.r - a.r) * ratio, 0.0f, 255.0f));
        result.g = static_cast<uint8_t>(Mathematics::Constrain(a.g + (b.g - a.g) * ratio, 0.0f, 255.0f));
        result.b = static_cast<uint8_t>(Mathematics::Constrain(a.b + (b.b - a.b) * ratio, 0.0f, 255.0f));
        return result;
    }

    /**
     * @brief Linear interpolation between two colors.
     */
    static Color888 Lerp(const Color888& a, const Color888& b, float t) {
        t = Mathematics::Constrain(t, 0.0f, 1.0f);
        return Color888(
            static_cast<uint8_t>(a.r + (b.r - a.r) * t),
            static_cast<uint8_t>(a.g + (b.g - a.g) * t),
            static_cast<uint8_t>(a.b + (b.b - a.b) * t)
        );
    }

    /**
     * @brief Create a Color888 from HSV values.
     * @param h Hue in degrees (0-360)
     * @param s Saturation (0.0 - 1.0)
     * @param v Value/brightness (0.0 - 1.0)
     */
    static Color888 FromHSV(float h, float s, float v);

    /**
     * @brief Convert to string representation.
     */
    UString ToString() const {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "RGB888(%d, %d, %d)", r, g, b);
        return UString(buffer);
    }

    // Operators
    Color888 operator+(const Color888& other) const {
        return Color888(
            Mathematics::Min(r + other.r, 255),
            Mathematics::Min(g + other.g, 255),
            Mathematics::Min(b + other.b, 255)
        );
    }

    Color888& operator+=(const Color888& other) {
        r = Mathematics::Min(r + other.r, 255);
        g = Mathematics::Min(g + other.g, 255);
        b = Mathematics::Min(b + other.b, 255);
        return *this;
    }

    Color888 operator-(const Color888& other) const {
        return Color888(
            r > other.r ? r - other.r : 0,
            g > other.g ? g - other.g : 0,
            b > other.b ? b - other.b : 0
        );
    }

    Color888& operator-=(const Color888& other) {
        r = r > other.r ? r - other.r : 0;
        g = g > other.g ? g - other.g : 0;
        b = b > other.b ? b - other.b : 0;
        return *this;
    }

    Color888 operator*(float scalar) const {
        return Color888(
            static_cast<uint8_t>(Mathematics::Constrain(r * scalar, 0.0f, 255.0f)),
            static_cast<uint8_t>(Mathematics::Constrain(g * scalar, 0.0f, 255.0f)),
            static_cast<uint8_t>(Mathematics::Constrain(b * scalar, 0.0f, 255.0f))
        );
    }

    Color888& operator*=(float scalar) {
        r = static_cast<uint8_t>(Mathematics::Constrain(r * scalar, 0.0f, 255.0f));
        g = static_cast<uint8_t>(Mathematics::Constrain(g * scalar, 0.0f, 255.0f));
        b = static_cast<uint8_t>(Mathematics::Constrain(b * scalar, 0.0f, 255.0f));
        return *this;
    }

    Color888 operator*(const Color888& other) const {
        return Color888(
            (r * other.r) / 255,
            (g * other.g) / 255,
            (b * other.b) / 255
        );
    }

    Color888& operator*=(const Color888& other) {
        r = (r * other.r) / 255;
        g = (g * other.g) / 255;
        b = (b * other.b) / 255;
        return *this;
    }

    bool operator==(const Color888& other) const {
        return r == other.r && g == other.g && b == other.b;
    }

    bool operator!=(const Color888& other) const {
        return !(*this == other);
    }

    // Reflection
    KL_BEGIN_FIELDS(Color888)
        KL_FIELD(Color888, r, "R", 0, 255),
        KL_FIELD(Color888, g, "G", 0, 255),
        KL_FIELD(Color888, b, "B", 0, 255)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Color888)
        KL_METHOD_AUTO(Color888, Set, "Set color"),
        KL_METHOD_AUTO(Color888, Scale, "Scale brightness"),
        KL_METHOD_AUTO(Color888, Add, "Add value"),
        KL_METHOD_AUTO(Color888, HueShift, "Hue shift"),
        KL_SMETHOD_AUTO(Color888::Lerp, "Lerp"),
        KL_SMETHOD_AUTO(Color888::FromHSV, "Create from HSV"),
        KL_METHOD_AUTO(Color888, ToString, "To string")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Color888)
        KL_CTOR0(Color888),
        KL_CTOR(Color888, uint8_t, uint8_t, uint8_t),
        KL_CTOR(Color888, const Color888 &),
        KL_CTOR(Color888, const Vector3D &)
    KL_END_DESCRIBE(Color888)
};

} // namespace koilo
