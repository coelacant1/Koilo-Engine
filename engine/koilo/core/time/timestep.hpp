// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file TimeStep.h
 * @brief Utility class for timing operations based on a set frequency.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <stdint.h>
#include <koilo/core/platform/time.hpp>
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @class TimeStep
 * @brief Provides a mechanism to trigger actions at a specified frequency.
 */
class TimeStep {
private:
    uint32_t previousMillis; ///< Stores the last recorded time in milliseconds.
    uint16_t updateInterval;      ///< Interval in milliseconds between updates.

public:
    /**
     * @brief Constructor to initialize TimeStep with a frequency.
     * @param frequency The frequency in Hz.
     */
    TimeStep(float frequency);

    /**
     * @brief Sets the frequency for the TimeStep.
     * @param frequency The new frequency in Hz.
     */
    void SetFrequency(float frequency);

    /**
     * @brief Checks if the specified time interval has elapsed.
     * @return True if the interval has elapsed, otherwise false.
     */
    bool IsReady();

    KL_BEGIN_FIELDS(TimeStep)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(TimeStep)
        KL_METHOD_AUTO(TimeStep, SetFrequency, "Set frequency"),
        KL_METHOD_AUTO(TimeStep, IsReady, "Is ready")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(TimeStep)
        KL_CTOR(TimeStep, float)
    KL_END_DESCRIBE(TimeStep)

};

} // namespace koilo
