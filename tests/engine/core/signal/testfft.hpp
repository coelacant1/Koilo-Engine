/**
 * @file testfft.hpp
 * @brief Unit tests for the FFT class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/fft.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestFFT
 * @brief Contains static test methods for the FFT class.
 */
class TestFFT {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSize();
    static void TestForward();
    static void TestInverse();
    static void TestComplexMagnitude();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
