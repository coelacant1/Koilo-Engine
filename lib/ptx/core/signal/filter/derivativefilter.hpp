/**
 * @file DerivativeFilter.h
 * @brief Provides a `DerivativeFilter` class for calculating the rate of change of input values.
 *
 * The `DerivativeFilter` combines a running average filter and a minimum filter to normalize
 * and constrain the derivative values, ensuring stability and noise reduction.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "runningaveragefilter.hpp" // Includes the running average filter utility.
#include "minfilter.hpp" // Includes the minimum filter utility.
#include "../../math/mathematics.hpp" // Includes mathematical utilities for constraints and operations.
#include "../../../registry/reflect_macros.hpp"

/**
 * @class DerivativeFilter
 * @brief Calculates the derivative (rate of change) of input values with filtering for stability.
 *
 * The `DerivativeFilter` smooths the derivative output using a running average filter
 * and normalizes it using a minimum filter to prevent rapid fluctuations.
 */
class DerivativeFilter {
private:
    RunningAverageFilter<10> output = RunningAverageFilter<10>(0.2f); ///< Running average filter for smoothing the derivative output.
    MinFilter<40> minFilter; ///< Minimum filter for baseline normalization.
    float previousReading = 0.0f; ///< Stores the previous input value for calculating the rate of change.
    float outputValue = 0.0f; ///< Stores the most recent filtered derivative value.

public:
    /**
     * @brief Constructs a `DerivativeFilter` instance with default configurations.
     */
    DerivativeFilter();

    /**
     * @brief Retrieves the current filtered derivative output.
     * 
     * @return The filtered derivative value.
     */
    float GetOutput();

    /**
     * @brief Filters the derivative of the input value and normalizes the output.
     *
     * @param value The current input value.
     * @return The filtered and normalized derivative value.
     */
    float Filter(float value);

    PTX_BEGIN_FIELDS(DerivativeFilter)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(DerivativeFilter)
        PTX_METHOD_AUTO(DerivativeFilter, GetOutput, "Get output"),
        PTX_METHOD_AUTO(DerivativeFilter, Filter, "Filter")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(DerivativeFilter)
        PTX_CTOR0(DerivativeFilter)
    PTX_END_DESCRIBE(DerivativeFilter)

};
