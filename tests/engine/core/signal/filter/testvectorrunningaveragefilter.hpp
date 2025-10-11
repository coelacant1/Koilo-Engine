/**
 * @file testvectorrunningaveragefilter.hpp
 * @brief Unit tests for the VectorRunningAverageFilter class.
 *
 * Tests cover 3D vector filtering with constructor validation, independent
 * component filtering, steady-state behavior, transient response, and edge cases.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/filter/vectorrunningaveragefilter.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestVectorRunningAverageFilter
 * @brief Contains static test methods for the VectorRunningAverageFilter class.
 */
class TestVectorRunningAverageFilter {
public:
    // Constructor & lifecycle tests
    static void TestParameterizedConstructor();

    // Method tests
    static void TestFilter();
    static void TestSetGain();
    static void TestReset();
    static void TestGetCapacity();

    // Vector filtering tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestDefaultConstructor();
    static void RunAllTests();
};
