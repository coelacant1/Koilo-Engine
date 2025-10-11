/**
 * @file testfftvoicedetection.hpp
 * @brief Unit tests for the FFTVoiceDetection class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/fftvoicedetection.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestFFTVoiceDetection
 * @brief Contains static test methods for the FFTVoiceDetection class.
 */
class TestFFTVoiceDetection {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetThreshold();
    static void TestGetViseme();
    static void TestToString();
    static void TestResetVisemes();
    static void TestUpdate();
    static void TestGetPeakCount();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
