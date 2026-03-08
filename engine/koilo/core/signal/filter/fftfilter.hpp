// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file FFTFilter.h
 * @brief Provides the `FFTFilter` class for processing and normalizing FFT data.
 *
 * The `FFTFilter` applies a smoothing filter to FFT data and normalizes the output
 * to constrain the values within a specified range.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "runningaveragefilter.hpp" // Includes the running average filter utility.
#include <koilo/core/math/mathematics.hpp> // Includes mathematical utilities for constraints and operations.
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @class FFTFilter
 * @brief Processes and normalizes FFT data.
 *
 * The `FFTFilter` applies a running average filter to FFT input values, removes
 * a baseline minimum value, and normalizes the output for stability and usability
 * in other FFT-based functionalities.
 */
class FFTFilter {
private:
    RunningAverageFilter minKF{20, 0.05f}; ///< Running average filter for baseline normalization.
    float outputValue = 0.0f; ///< Stores the most recent filtered output value.

public:
    /**
     * @brief Constructs an `FFTFilter` instance with default configurations.
     */
    FFTFilter();

    /**
     * @brief Retrieves the current filtered and normalized output value.
     * 
     * @return The most recent filtered output value.
     */
    float GetOutput();

    /**
     * @brief Filters and normalizes the input value for FFT data processing.
     *
     * @param value The current FFT input value.
     * @return The filtered and normalized FFT value.
     */
    float Filter(float value);

    KL_BEGIN_FIELDS(FFTFilter)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(FFTFilter)
        KL_METHOD_AUTO(FFTFilter, GetOutput, "Get output"),
        KL_METHOD_AUTO(FFTFilter, Filter, "Filter")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(FFTFilter)
        KL_CTOR0(FFTFilter)
    KL_END_DESCRIBE(FFTFilter)

};

} // namespace koilo
