/**
 * @file MinFilter.h
 * @brief Implements a moving minimum filter for processing data streams.
 *
 * The `MinFilter` maintains a window of the smallest values over a specified
 * memory range. It is useful for smoothing data while capturing minimum values.
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
 * @class MinFilter
 * @brief Implements a minimum filter over a sliding window.
 *
 * This class filters input data by maintaining a history of minimum values
 * over a specified memory window. It is particularly useful for detecting
 * and tracking minimum values in data streams.
 */
class MinFilter {
private:
    size_t capacity;                   ///< Size of the sliding window.
    size_t blockCount;                 ///< Number of blocks used for minimum tracking.
    std::vector<float> values;         ///< Circular buffer of values.
    std::vector<float> minValues;      ///< Minimum values per block.
    size_t currentAmount = 0;          ///< Number of values currently stored in the buffer.
    bool ignoreSame;                   ///< Whether to ignore consecutive identical values.

    /**
     * @brief Shifts the values in the provided array to make room for a new value.
     */
    static void ShiftArray(std::vector<float>& arr);

public:
    /**
     * @brief Constructs a `MinFilter` with the specified memory size and behavior.
     *
     * @param memory The number of samples to preserve in the sliding window (defaults to 40).
     * @param ignoreSame Whether to ignore consecutive identical values (default: true).
     */
    explicit MinFilter(size_t memory = 40, bool ignoreSame = true);

    /**
     * @brief Filters the given value, updating the minimum value within the memory window.
     *
     * @param value The input value to filter.
     * @return The minimum value within the memory window after updating with the new value.
     */
    float Filter(float value);

    /**
     * @brief Resets the filter to an initial state filled with zeros.
     */
    void Reset();

    /**
     * @brief Returns the configured capacity of the filter.
     */
    size_t GetCapacity() const { return capacity; }

    PTX_BEGIN_FIELDS(MinFilter)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(MinFilter)
        PTX_METHOD_AUTO(MinFilter, Filter, "Filter"),
        PTX_METHOD_AUTO(MinFilter, Reset, "Reset"),
        PTX_METHOD_AUTO(MinFilter, GetCapacity, "Get capacity")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(MinFilter)
        PTX_CTOR(MinFilter, int, bool)
    PTX_END_DESCRIBE(MinFilter)

};
