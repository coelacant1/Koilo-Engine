/**
 * @file testderivativefilter.hpp
 * @brief Unit tests for the DerivativeFilter class.
 *
 * Tests cover rate of change calculation, derivative detection for various
 * signal patterns (constant, linear ramp, sinusoidal), normalization, and edge cases.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/filter/derivativefilter.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestDerivativeFilter
 * @brief Contains static test methods for the DerivativeFilter class.
 */
class TestDerivativeFilter {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();

    // Method tests
    static void TestGetOutput();
    static void TestFilter();

    // Derivative behavior tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestParameterizedConstructor();
    static void RunAllTests();
};
