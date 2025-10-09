/**
 * @file VectorRunningAverageFilter.h
 * @brief Implements a running average filter for 3D vector smoothing.
 *
 * The `VectorRunningAverageFilter` class applies independent running average filters
 * to each component (X, Y, Z) of a 3D vector to reduce noise in the input data.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "runningaveragefilter.hpp"
#include "../../math/vector3d.hpp" // Includes mathematical utilities for constraints and operations.
#include "../../../registry/reflect_macros.hpp"

/**
 * @class VectorRunningAverageFilter
 * @brief Applies running average filtering independently to each component of a 3D vector.
 *
 * The `VectorRunningAverageFilter` class uses three `RunningAverageFilter` instances
 * to process the X, Y, and Z components of a `Vector3D`, providing a smoothed output for noisy 3D data.
 *
 */
class VectorRunningAverageFilter {
private:
    RunningAverageFilter X; ///< Running average filter for the X component of the vector.
    RunningAverageFilter Y; ///< Running average filter for the Y component of the vector.
    RunningAverageFilter Z; ///< Running average filter for the Z component of the vector.
    size_t capacity;        ///< Memory depth configured for each component filter.

public:
    /**
     * @brief Constructs a `VectorRunningAverageFilter` with runtime memory depth and optional gain.
     *
     * Initializes three running average filters-one per axis-with the provided configuration.
     *
     * @param memory The number of samples remembered for each axis.
     * @param gain The smoothing gain for the running average filter (range 0.0 to 1.0).
     */
    VectorRunningAverageFilter(size_t memory, float gain = 0.1f);

    /**
     * @brief Filters a 3D vector input using the running average filter.
     *
     * Applies running average filtering independently to the X, Y, and Z components of the input vector.
     *
     * @param input The input `Vector3D` to filter.
     * @return The filtered `Vector3D`.
     */
    Vector3D Filter(Vector3D input);

    /**
     * @brief Adjusts the gain on all component filters simultaneously.
     */
    void SetGain(float gain);

    /**
     * @brief Clears accumulated samples across all component filters.
     */
    void Reset();

    /**
     * @brief Returns the configured memory depth.
     */
    size_t GetCapacity() const { return capacity; }

    PTX_BEGIN_FIELDS(VectorRunningAverageFilter)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(VectorRunningAverageFilter)
        PTX_METHOD_AUTO(VectorRunningAverageFilter, Filter, "Filter"),
        PTX_METHOD_AUTO(VectorRunningAverageFilter, SetGain, "Set gain"),
        PTX_METHOD_AUTO(VectorRunningAverageFilter, Reset, "Reset"),
        PTX_METHOD_AUTO(VectorRunningAverageFilter, GetCapacity, "Get capacity")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(VectorRunningAverageFilter)
        PTX_CTOR(VectorRunningAverageFilter, int, float)
    PTX_END_DESCRIBE(VectorRunningAverageFilter)

};
