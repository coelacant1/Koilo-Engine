/**
 * @file testkalmanfilter.hpp
 * @brief Unit tests for the KalmanFilter class.
 *
 * This test suite covers comprehensive unit testing of the KalmanFilter class,
 * including constructor validation, filter operations (predict and update steps),
 * state estimation accuracy, covariance evolution, getter/setter methods,
 * and edge cases such as zero noise and extreme values.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/filter/kalmanfilter.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestKalmanFilter
 * @brief Contains static test methods for the KalmanFilter class.
 */
class TestKalmanFilter {
public:
    // Constructor & lifecycle tests
    static void TestParameterizedConstructor();

    // Method tests
    static void TestReset();
    static void TestFilter();
    static void TestGetEstimation();
    static void TestGetProcessNoise();
    static void TestGetSensorNoise();
    static void TestGetErrorCovariance();
    static void TestSetProcessNoise();
    static void TestSetSensorNoise();
    static void TestSetErrorCovariance();

    // Kalman filter behavior tests

    // Edge case & integration tests

    /**
     * @brief Runs all test methods.
     */
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void RunAllTests();
};
