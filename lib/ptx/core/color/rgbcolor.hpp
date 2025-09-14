/**
 * @file RGBColor.h
 * @brief Defines the RGBColor class for managing and manipulating RGB color values.
 *
 * The RGBColor class provides functionality to represent and manipulate RGB color values,
 * including scaling brightness, adding values, hue shifting, and interpolating between colors.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <cstdint>

#include "../math/mathematics.hpp"
#include "../math/quaternion.hpp"
#include "../math/vector3d.hpp"
#include "../platform/ustring.hpp"

#include "../../registry/reflect_macros.hpp"

/**
 * @class RGBColor
 * @brief Represents an RGB color and provides methods for manipulation.
 */
class RGBColor{
public:
    uint8_t R = 0; ///< Red component of the color (0-255).
    uint8_t G = 0; ///< Green component of the color (0-255).
    uint8_t B = 0; ///< Blue component of the color (0-255).

    /**
     * @brief Default constructor initializes the color to black (0, 0, 0).
     */
    RGBColor();

    /**
     * @brief Constructor to initialize RGBColor with specified R, G, and B values.
     * @param R Red component (0-255).
     * @param G Green component (0-255).
     * @param B Blue component (0-255).
     */
    RGBColor(const uint8_t& R, const uint8_t& G, const uint8_t& B);

    /**
     * @brief Copy constructor.
     * @param rgbColor Reference to an existing RGBColor object.
     */
    RGBColor(const RGBColor& rgbColor);

    /**
     * @brief Constructor to initialize RGBColor from a Vector3D.
     * @param color Vector3D representing RGB values.
     */
    RGBColor(const Vector3D& color);

    /**
     * @brief Sets the RGB values of the color.
     * @param R Red component (0-255).
     * @param G Green component (0-255).
     * @param B Blue component (0-255).
     */
    void SetColor(const uint8_t& R, const uint8_t& G, const uint8_t& B);

    /**
     * @brief Scales the brightness of the color to a maximum value.
     * @param maxBrightness The maximum brightness value (0-255).
     * @return A new RGBColor with scaled brightness.
     */
    RGBColor Scale(const uint8_t& maxBrightness);

    /**
     * @brief Adds a value to each RGB component of the color.
     * @param value The value to add (0-255).
     * @return A new RGBColor with modified values.
     */
    RGBColor Add(const uint8_t& value);

    /**
     * @brief Shifts the hue of the color by a specified angle in degrees.
     * @param hueDeg The angle in degrees to shift the hue.
     * @return A new RGBColor with the hue shifted.
     */
    RGBColor HueShift(const float& hueDeg);

    /**
     * @brief Interpolates between two colors based on a ratio.
     * @param a The starting color.
     * @param b The ending color.
     * @param ratio A value between 0 and 1 representing the interpolation factor.
     * @return The interpolated RGBColor.
     */
    static RGBColor InterpolateColors(const RGBColor& a, const RGBColor& b, const float& ratio);

    /**
     * @brief Converts the RGBColor to a string representation.
     * @return A string in the format "(R, G, B)".
     */
    ptx::UString ToString();

    /**
     * @brief Adds another color to this one component-wise.
     * @param other The RGBColor to add.
     * @return A new RGBColor representing the sum, clamped to 255.
     */
    RGBColor operator+(const RGBColor& other) const;

    /**
     * @brief In-place addition of another color.
     * @param other The RGBColor to add.
     * @return A reference to this modified RGBColor.
     */
    RGBColor& operator+=(const RGBColor& other);

    /**
     * @brief Subtracts another color from this one component-wise.
     * @param other The RGBColor to subtract.
     * @return A new RGBColor representing the difference, clamped to 0.
     */
    RGBColor operator-(const RGBColor& other) const;

    /**
     * @brief In-place subtraction of another color.
     * @param other The RGBColor to subtract.
     * @return A reference to this modified RGBColor.
     */
    RGBColor& operator-=(const RGBColor& other);


    /**
     * @brief Scales the color by a float value (component-wise).
     * @param scalar The scaling factor.
     * @return A new, scaled RGBColor.
     */
    RGBColor operator*(const float& scalar) const;

    /**
     * @brief Modulates this color with another (component-wise multiplication).
     * @param other The color to multiply with.
     * @return A new, modulated RGBColor.
     */
    RGBColor operator*(const RGBColor& other) const;

    /**
     * @brief In-place scaling of the color by a float value.
     * @param scalar The scaling factor.
     * @return A reference to this modified RGBColor.
     */
    RGBColor& operator*=(const float& scalar);

    /**
     * @brief In-place modulation of this color with another.
     * @param other The color to multiply with.
     * @return A reference to this modified RGBColor.
     */
    RGBColor& operator*=(const RGBColor& other);

    /**
     * @brief Divides the color by a float value (component-wise).
     * @param scalar The divisor.
     * @return A new, scaled RGBColor.
     */
    RGBColor operator/(const float& scalar) const;

    /**
     * @brief Divides this color by another (un-modulates).
     * @param other The RGBColor to divide by.
     * @return A new RGBColor representing the result.
     */
    RGBColor operator/(const RGBColor& other) const;

    /**
     * @brief In-place division by a float value.
     * @param scalar The divisor.
     * @return A reference to this modified RGBColor.
     */
    RGBColor& operator/=(const float& scalar);

    /**
     * @brief In-place division by another color.
     * @param other The RGBColor to divide by.
     * @return A reference to this modified RGBColor.
     */
    RGBColor& operator/=(const RGBColor& other);

    // -------- Reflection: Fields --------
    PTX_BEGIN_FIELDS(RGBColor)
        PTX_FIELD(RGBColor, R, "Red",   0, 255),
        PTX_FIELD(RGBColor, G, "Green", 0, 255),
        PTX_FIELD(RGBColor, B, "Blue",  0, 255)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(RGBColor)
        PTX_METHOD_AUTO(RGBColor, Add,      "Add brightness"),
        PTX_METHOD_AUTO(RGBColor, Scale,    "Scale"),
        PTX_METHOD_AUTO(RGBColor, HueShift, "Hue shift"),
        PTX_METHOD_AUTO(RGBColor, ToString, "ToString"),
        PTX_SMETHOD_AUTO(RGBColor::InterpolateColors, "Lerp two colors")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(RGBColor)
        PTX_CTOR0(RGBColor),
        PTX_CTOR(RGBColor, const uint8_t&, const uint8_t&, const uint8_t&)
    PTX_END_DESCRIBE(RGBColor)
};
