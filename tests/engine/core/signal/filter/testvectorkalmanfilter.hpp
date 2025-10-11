/**
 * @file testvectorkalmanfilter.hpp
 * @brief Unit tests for the VectorKalmanFilter class.
 *
 * This test suite covers comprehensive unit testing of the VectorKalmanFilter class,
 * which applies independent Kalman filtering to each component (X, Y, Z) of a 3D vector.
 * Tests include constructor validation, filter operations, state estimation accuracy,
 * convergence behavior, and edge cases for 3D vector filtering.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/filter/vectorkalmanfilter.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestVectorKalmanFilter
 * @brief Contains static test methods for the VectorKalmanFilter class.
 */
class TestVectorKalmanFilter {
public:
    // Constructor & lifecycle tests
    static void TestParameterizedConstructor();

    // Method tests
    static void TestFilter();
    static void TestReset();

    // Vector Kalman filter behavior tests

    // Edge case & integration tests

    /**
     * @brief Runs all test methods.
     */
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void RunAllTests();
};
