/**
 * @file testrampfilter.hpp
 * @brief Unit tests for the RampFilter class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/filter/rampfilter.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRampFilter
 * @brief Contains static test methods for the RampFilter class.
 */
class TestRampFilter {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestFilter();
    static void TestSetIncrement();
    static void TestSetFrames();

    // Ramp behavior tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
