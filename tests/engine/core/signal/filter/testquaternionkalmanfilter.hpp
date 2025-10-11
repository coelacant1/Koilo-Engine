/**
 * @file testquaternionkalmanfilter.hpp
 * @brief Unit tests for the QuaternionKalmanFilter class.
 *
 * This test suite covers comprehensive unit testing of the QuaternionKalmanFilter class,
 * which provides smoothing and filtering for quaternion (rotation) data. Tests include
 * constructor validation, filter operations, memory-based smoothing behavior,
 * quaternion normalization preservation, and edge cases for rotation filtering.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/filter/quaternionkalmanfilter.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestQuaternionKalmanFilter
 * @brief Contains static test methods for the QuaternionKalmanFilter class.
 */
class TestQuaternionKalmanFilter {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestFilter();

    // Quaternion filter behavior tests

    // Edge case & integration tests

    /**
     * @brief Runs all test methods.
     */
    static void TestEdgeCases();
    static void RunAllTests();
};
