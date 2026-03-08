// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file QuaternionKalmanFilter.h
 * @brief Implements a Kalman filter for smoothing quaternion data.
 *
 * The `QuaternionKalmanFilter` class provides a mechanism to smooth quaternion values
 * using a simple Kalman filter approach.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <vector>

#include <koilo/core/math/quaternion.hpp>
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @class QuaternionKalmanFilter
 * @brief A Kalman filter for quaternion smoothing.
 *
 * This class applies a Kalman filter to quaternion data to reduce noise and
 * smooth transitions between rotations.
 */
class QuaternionKalmanFilter {
private:
    float gain; ///< The filter gain, controls the weight of new data versus the estimated state.
    int memory; ///< The size of the internal buffer to store quaternion history.
    std::vector<Quaternion> values; ///< Circular buffer of quaternion values for the filter's memory.

    int currentAmount = 0; ///< Tracks the current number of quaternions stored in memory.

    /**
     * @brief Shifts the stored quaternions to remove the oldest entry.
     */
    void ShiftArray();

public:
    /**
     * @brief Default constructor for `QuaternionKalmanFilter`.
     *
     * Initializes the filter with default parameters.
     */
    QuaternionKalmanFilter();

    /**
     * @brief Constructs a `QuaternionKalmanFilter` with specified parameters.
     *
     * @param gain The filter gain.
     * @param memory The size of the filter's internal memory.
     */
    QuaternionKalmanFilter(float gain, int memory);

    /**
     * @brief Filters a quaternion value to reduce noise.
     *
     * @param value The new quaternion value to process.
     * @return The filtered quaternion.
     */
    Quaternion Filter(Quaternion value);

    /**
     * @brief Destructor for `QuaternionKalmanFilter`.
     *
     * Trivial (uses RAII); present for ABI / reflection completeness.
     */
    ~QuaternionKalmanFilter();

    KL_BEGIN_FIELDS(QuaternionKalmanFilter)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(QuaternionKalmanFilter)
        KL_METHOD_AUTO(QuaternionKalmanFilter, Filter, "Filter")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(QuaternionKalmanFilter)
        KL_CTOR0(QuaternionKalmanFilter),
        KL_CTOR(QuaternionKalmanFilter, float, int)
    KL_END_DESCRIBE(QuaternionKalmanFilter)

};

} // namespace koilo
