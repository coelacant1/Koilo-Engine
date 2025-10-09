/**
 * @file KalmanFilter.h
 * @brief Provides a template-based implementation of a Kalman Filter.
 *
 * The Kalman Filter is used for estimating the state of a system in the presence
 * of noise. It is widely used in signal processing, control systems, and navigation.
 *
 * @date 22/12/2024
 * @version 1.0
 * @author Coela Can't
 */

#pragma once

#include "../../../registry/reflect_macros.hpp"
/**
 * @class KalmanFilter
 * @brief Implements a 1D Kalman Filter with runtime-configurable parameters.
 *
 * The runtime version mirrors the original template behaviour but stores state
 * using single-precision floats to keep the implementation in a translation unit.
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

    PTX_BEGIN_FIELDS(KalmanFilter)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(KalmanFilter)
        PTX_METHOD_AUTO(KalmanFilter, Reset, "Reset"),
        PTX_METHOD_AUTO(KalmanFilter, Filter, "Filter"),
        PTX_METHOD_AUTO(KalmanFilter, GetEstimation, "Get estimation"),
        PTX_METHOD_AUTO(KalmanFilter, GetProcessNoise, "Get process noise"),
        PTX_METHOD_AUTO(KalmanFilter, GetSensorNoise, "Get sensor noise"),
        PTX_METHOD_AUTO(KalmanFilter, GetErrorCovariance, "Get error covariance"),
        PTX_METHOD_AUTO(KalmanFilter, SetProcessNoise, "Set process noise"),
        PTX_METHOD_AUTO(KalmanFilter, SetSensorNoise, "Set sensor noise"),
        PTX_METHOD_AUTO(KalmanFilter, SetErrorCovariance, "Set error covariance")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(KalmanFilter)
        PTX_CTOR(KalmanFilter, float, float, float)
    PTX_END_DESCRIBE(KalmanFilter)

};
