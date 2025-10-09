/**
 * @file MaxFilter.h
 * @brief Implements a moving maximum filter for processing data streams.
 *
 * The `MaxFilter` maintains a window of the largest values over a specified
 * memory range. It can be used to smooth data while capturing peak values.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <cstddef>
#include <vector>
#include "../../../registry/reflect_macros.hpp"

/**
 * @class MaxFilter
 * @brief Implements a maximum filter over a sliding window.
 *
 * This class filters input data by maintaining a history of maximum values
 * over a specified memory window. It is particularly useful for detecting
 * and tracking peak values in data streams.
 */
class MaxFilter {
private:
    size_t capacity;              ///< Size of the sliding window.
    size_t blockCount;            ///< Number of blocks used for peak tracking.
    std::vector<float> values;    ///< Circular buffer of values.
    std::vector<float> maxValues; ///< Maximum values per block.
    size_t currentAmount = 0;     ///< Number of values currently stored in the buffer.

    /**
     * @brief Shifts the values in the provided array to make room for a new value.
     */
    static void ShiftArray(std::vector<float>& arr);

public:
    /**
     * @brief Constructs a `MaxFilter` with the specified memory size.
     *
     * @param memory The number of samples to preserve in the sliding window (defaults to 40).
     */
    explicit MaxFilter(size_t memory = 40);

    /**
     * @brief Filters the given value, updating the maximum value within the memory window.
     *
     * @param value The input value to filter.
     * @return The maximum value within the memory window after updating with the new value.
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

    PTX_BEGIN_FIELDS(MaxFilter)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(MaxFilter)
        PTX_METHOD_AUTO(MaxFilter, Filter, "Filter"),
        PTX_METHOD_AUTO(MaxFilter, Reset, "Reset"),
        PTX_METHOD_AUTO(MaxFilter, GetCapacity, "Get capacity")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(MaxFilter)
        PTX_CTOR(MaxFilter, int)
    PTX_END_DESCRIBE(MaxFilter)

};
