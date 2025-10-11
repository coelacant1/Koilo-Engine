/**
 * @file testmaxfilter.hpp
 * @brief Unit tests for the MaxFilter class.
 *
 * Tests cover sliding window maximum tracking, peak detection,
 * window behavior, and edge cases with various signal patterns.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/filter/maxfilter.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMaxFilter
 * @brief Contains static test methods for the MaxFilter class.
 */
class TestMaxFilter {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestFilter();
    static void TestReset();
    static void TestGetCapacity();

    // Max filter behavior tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
