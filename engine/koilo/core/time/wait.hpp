// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file Wait.h
 * @brief Utility class for handling non-blocking wait operations.
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
 * @class Wait
 * @brief Provides a mechanism to wait for a specified duration without blocking.
 */
class Wait {
private:
    uint32_t previousMillis; ///< Stores the last recorded time in milliseconds.
    uint32_t millisToWait;   ///< Duration to wait in milliseconds.

public:
    /**
     * @brief Constructor to initialize Wait with a duration.
     * @param millisToWait The duration to wait in milliseconds.
     */
    Wait(uint32_t millisToWait);

    /**
     * @brief Resets the wait timer to start counting again.
     */
    void Reset();

    /**
     * @brief Checks if the specified duration has elapsed.
     * @return True if the duration has elapsed, otherwise false.
     */
    bool IsFinished();

    KL_BEGIN_FIELDS(Wait)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(Wait)
        KL_METHOD_AUTO(Wait, Reset, "Reset"),
        KL_METHOD_AUTO(Wait, IsFinished, "Is finished")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Wait)
        KL_CTOR(Wait, uint32_t)
    KL_END_DESCRIBE(Wait)

};

} // namespace koilo
