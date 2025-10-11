/**
 * @file testrunningaveragefilter.hpp
 * @brief Unit tests for the RunningAverageFilter class.
 *
 * Tests cover constructor validation, filter behavior with various inputs,
 * steady-state convergence, transient response, edge cases, and gain adjustments.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/filter/runningaveragefilter.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRunningAverageFilter
 * @brief Contains static test methods for the RunningAverageFilter class.
 */
class TestRunningAverageFilter {
public:
    // Constructor & lifecycle tests
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetGain();
    static void TestFilter();
    static void TestReset();
    static void TestGetCapacity();
    static void TestGetGain();

    // Signal processing tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestDefaultConstructor();
    static void RunAllTests();
};
