// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file YawPitchRoll.h
 * @brief Represents an orientation using yaw, pitch, and roll angles.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "vector3d.hpp"
#include "mathematics.hpp"
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @class YawPitchRoll
 * @brief A class to represent yaw, pitch, and roll angles for orientation.
 */
class YawPitchRoll {
public:
    float Yaw;   ///< The yaw angle, representing rotation around the vertical axis.
    float Pitch; ///< The pitch angle, representing rotation around the lateral axis.
    float Roll;  ///< The roll angle, representing rotation around the longitudinal axis.

    /**
     * @brief Default constructor, initializes yaw, pitch, and roll to 0.
     */
    YawPitchRoll();

    /**
     * @brief Constructor to initialize yaw, pitch, and roll from a vector.
     * @param vector A Vector3D where X = yaw, Y = pitch, Z = roll.
     */
    YawPitchRoll(Vector3D vector);

    /**
     * @brief Copy constructor.
     * @param ypr The YawPitchRoll object to copy from.
     */
    YawPitchRoll(const YawPitchRoll& ypr);

    /**
     * @brief Constructor to initialize yaw, pitch, and roll with specific values.
     * @param yaw The yaw angle.
     * @param pitch The pitch angle.
     * @param roll The roll angle.
     */
    YawPitchRoll(float yaw, float pitch, float roll);

    /**
     * @brief Converts the yaw, pitch, and roll angles to a string representation.
     * @return A string representation of the YawPitchRoll object.
     */
    koilo::UString ToString() const;

    KL_BEGIN_FIELDS(YawPitchRoll)
        KL_FIELD(YawPitchRoll, Yaw, "Yaw", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(YawPitchRoll, Pitch, "Pitch", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(YawPitchRoll, Roll, "Roll", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(YawPitchRoll)
        KL_METHOD_AUTO(YawPitchRoll, ToString, "To string")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(YawPitchRoll)
        KL_CTOR0(YawPitchRoll),
        KL_CTOR(YawPitchRoll, Vector3D),
        KL_CTOR(YawPitchRoll, const YawPitchRoll &),
        KL_CTOR(YawPitchRoll, float, float, float)
    KL_END_DESCRIBE(YawPitchRoll)

};

} // namespace koilo
