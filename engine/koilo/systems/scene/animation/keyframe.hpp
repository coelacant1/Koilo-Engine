// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file KeyFrame.h
 * @brief Declares the KeyFrame class for representing individual animation keyframes.
 *
 * This file defines the KeyFrame class, which encapsulates the properties of a single
 * keyframe in an animation, including its time and value.
 *
 * @author Coela Can't
 * @date 22/12/2024
 */

#pragma once

#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class KeyFrame
 * @brief Represents a single keyframe in an animation.
 *
 * The KeyFrame class stores the time and value for a specific point in an animation.
 * It provides methods for initialization and updating keyframe data.
 */
class KeyFrame {
public:
    float Time = 0.0f; ///< The time of the keyframe.
    float Value = 0.0f; ///< The value of the keyframe.

    /**
     * @brief Default constructor.
     *
     * Constructs a KeyFrame object with default time and value (0.0f).
     */
    KeyFrame();

    /**
     * @brief Parameterized constructor.
     *
     * Constructs a KeyFrame object with specified time and value.
     *
     * @param time The time of the keyframe.
     * @param value The value of the keyframe.
     */
    KeyFrame(float time, float value);

    /**
     * @brief Sets the time and value of the keyframe.
     *
     * Updates the properties of the keyframe to the specified time and value.
     *
     * @param time The new time of the keyframe.
     * @param value The new value of the keyframe.
     */
    void Set(float time, float value);

    KL_BEGIN_FIELDS(KeyFrame)
        KL_FIELD(KeyFrame, Time, "Time", __FLT_MIN__, __FLT_MAX__),
        KL_FIELD(KeyFrame, Value, "Value", __FLT_MIN__, __FLT_MAX__)
    KL_END_FIELDS

    KL_BEGIN_METHODS(KeyFrame)
        KL_METHOD_AUTO(KeyFrame, Set, "Set")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KeyFrame)
        KL_CTOR0(KeyFrame),
        KL_CTOR(KeyFrame, float, float)
    KL_END_DESCRIBE(KeyFrame)

};

} // namespace koilo
