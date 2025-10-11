/**
 * @file testfftfilter.hpp
 * @brief Unit tests for the FFTFilter class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/filter/fftfilter.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestFFTFilter
 * @brief Contains static test methods for the FFTFilter class.
 */
class TestFFTFilter {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetOutput();
    static void TestFilter();

    // Normalization tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
