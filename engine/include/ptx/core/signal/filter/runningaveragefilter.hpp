/**
 * @file RunningAverageFilter.h
 * @brief Implements a running average filter for smoothing data.
 *
 * The `RunningAverageFilter` class uses a specified memory size to smooth
 * incoming data values over time, providing a low-pass filtering effect.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "../../../registry/reflect_macros.hpp"

/**
 * @class RunningAverageFilter
 * @brief Smooths data values using a weighted running average.
 *
 * The `RunningAverageFilter` class calculates a running average using a configurable
 * memory depth, applying a gain to control the influence of new values.
 *
 */
class RunningAverageFilter {
private:
    float gain;              ///< The gain factor for the filter, controlling smoothing intensity.
    std::vector<float> data; ///< Circular buffer storing the most recent samples.
    size_t capacity;         ///< Maximum number of samples remembered by the filter.
    size_t currentAmount;    ///< The current number of valid entries in the buffer.
    size_t nextIndex;        ///< Index where the next sample will be written.
    float runningSum;        ///< Cached sum of values currently in the buffer.

public:
    /**
     * @brief Constructs a `RunningAverageFilter` with runtime-configurable memory.
     *
     * @param memory The number of samples to retain when computing the average.
     * @param gainValue The gain factor for the filter (typically between 0.0 and 1.0).
     */
    RunningAverageFilter(size_t memory, float gainValue = 0.1f);

    /**
     * @brief Sets the gain for the filter.
     *
     * @param gain The new gain value, controlling smoothing intensity.
     */
    void SetGain(float gain);

    /**
     * @brief Filters the input value using the running average.
     *
     * Calculates the smoothed output based on the memory buffer and gain.
     *
     * @param value The new input value to filter.
     * @return The filtered output value.
     */
    float Filter(float value);

    /**
     * @brief Resets the internal state and clears historical samples.
     */
    void Reset();

    /**
     * @brief Returns the configured memory depth of the filter.
     */
    size_t GetCapacity() const { return capacity; }

    /**
     * @brief Retrieves the current gain factor.
     */
    float GetGain() const { return gain; }

    PTX_BEGIN_FIELDS(RunningAverageFilter)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(RunningAverageFilter)
        PTX_METHOD_AUTO(RunningAverageFilter, SetGain, "Set gain"),
        PTX_METHOD_AUTO(RunningAverageFilter, Filter, "Filter"),
        PTX_METHOD_AUTO(RunningAverageFilter, Reset, "Reset"),
        PTX_METHOD_AUTO(RunningAverageFilter, GetCapacity, "Get capacity"),
        PTX_METHOD_AUTO(RunningAverageFilter, GetGain, "Get gain")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(RunningAverageFilter)
        PTX_CTOR(RunningAverageFilter, int, float)
    PTX_END_DESCRIBE(RunningAverageFilter)

};
