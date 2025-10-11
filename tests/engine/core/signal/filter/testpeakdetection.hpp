/**
 * @file testpeakdetection.hpp
 * @brief Unit tests for the PeakDetection class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/filter/peakdetection.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPeakDetection
 * @brief Contains static test methods for the PeakDetection class.
 */
class TestPeakDetection {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestCalculate();
    static void TestReset();
    static void TestGetSampleSize();
    static void TestGetLag();
    static void TestGetThreshold();
    static void TestGetInfluence();

    // Peak detection tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
