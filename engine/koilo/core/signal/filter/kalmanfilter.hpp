// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file KalmanFilter.h
 * @brief Provides an implementation of a Kalman Filter.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class KalmanFilter
 * @brief Implements a 1D Kalman Filter with runtime-configurable parameters.
 */
class KalmanFilter {
private:
    float processNoise;      ///< Process noise variance.
    float sensorNoise;       ///< Sensor noise variance.
    float estimation;        ///< Current estimated value.
    float errorCovariance;   ///< Error covariance of the estimation.

public:
    KalmanFilter(float processNoise, float sensorNoise, float errorCovariance);

    /**
     * @brief Resets the filter state to a known estimation and covariance.
     */
    void Reset(float estimationValue = 0.0f, float errorCovarianceValue = 1.0f);

    /**
     * @brief Filters the given input value using the Kalman update equations.
     */
    float Filter(float value);

    float GetEstimation() const { return estimation; }
    float GetProcessNoise() const { return processNoise; }
    float GetSensorNoise() const { return sensorNoise; }
    float GetErrorCovariance() const { return errorCovariance; }

    void SetProcessNoise(float value) { processNoise = value; }
    void SetSensorNoise(float value) { sensorNoise = value; }
    void SetErrorCovariance(float value) { errorCovariance = value; }

    KL_BEGIN_FIELDS(KalmanFilter)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(KalmanFilter)
        KL_METHOD_AUTO(KalmanFilter, Reset, "Reset"),
        KL_METHOD_AUTO(KalmanFilter, Filter, "Filter"),
        KL_METHOD_AUTO(KalmanFilter, GetEstimation, "Get estimation"),
        KL_METHOD_AUTO(KalmanFilter, GetProcessNoise, "Get process noise"),
        KL_METHOD_AUTO(KalmanFilter, GetSensorNoise, "Get sensor noise"),
        KL_METHOD_AUTO(KalmanFilter, GetErrorCovariance, "Get error covariance"),
        KL_METHOD_AUTO(KalmanFilter, SetProcessNoise, "Set process noise"),
        KL_METHOD_AUTO(KalmanFilter, SetSensorNoise, "Set sensor noise"),
        KL_METHOD_AUTO(KalmanFilter, SetErrorCovariance, "Set error covariance")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KalmanFilter)
        KL_CTOR(KalmanFilter, float, float, float)
    KL_END_DESCRIBE(KalmanFilter)

};

} // namespace koilo
