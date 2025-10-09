/**
 * @file VectorKalmanFilter.h
 * @brief Implements a Kalman filter for 3D vector smoothing.
 *
 * The `VectorKalmanFilter` class applies independent Kalman filters to each component (X, Y, Z)
 * of a 3D vector to smooth noisy data.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <ptx/core/signal/filter/kalmanfilter.hpp>
#include <ptx/core/math/vector3d.hpp>
#include "../../../registry/reflect_macros.hpp"

/**
 * @class VectorKalmanFilter
 * @brief Applies Kalman filtering independently to each component of a 3D vector.
 *
 * The `VectorKalmanFilter` class uses three `KalmanFilter` instances to process the
 * X, Y, and Z components of a `Vector3D`, providing a smooth output for noisy 3D data.
 *
 */
class VectorKalmanFilter {
private:
     KalmanFilter X; ///< Kalman filter for the X component of the vector.
     KalmanFilter Y; ///< Kalman filter for the Y component of the vector.
     KalmanFilter Z; ///< Kalman filter for the Z component of the vector.

public:
    /**
     * @brief Constructs a `VectorKalmanFilter` with specified parameters.
     *
     * Initializes the Kalman filters for each component with the given process noise,
     * sensor noise, and error covariance.
     *
     * @param processNoise The process noise for the Kalman filter.
     * @param sensorNoise The sensor noise for the Kalman filter.
     * @param errorCovariance The initial error covariance for the Kalman filter.
     */
    VectorKalmanFilter(float processNoise, float sensorNoise, float errorCovariance);

    /**
     * @brief Filters a 3D vector input using the Kalman filter.
     *
     * Applies Kalman filtering independently to the X, Y, and Z components of the input vector.
     *
     * @param input The input `Vector3D` to filter.
     * @return The filtered `Vector3D`.
     */
    Vector3D Filter(const Vector3D& input);

    void Reset(Vector3D estimation = Vector3D(), float errorCovariance = 1.0f);

    PTX_BEGIN_FIELDS(VectorKalmanFilter)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(VectorKalmanFilter)
        PTX_METHOD_AUTO(VectorKalmanFilter, Filter, "Filter"),
        PTX_METHOD_AUTO(VectorKalmanFilter, Reset, "Reset")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(VectorKalmanFilter)
        PTX_CTOR(VectorKalmanFilter, float, float, float)
    PTX_END_DESCRIBE(VectorKalmanFilter)

};
