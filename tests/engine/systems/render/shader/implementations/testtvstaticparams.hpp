/**
 * @file testtvstaticparams.hpp
 * @brief Unit tests for the TVStaticParams class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/shader/implementations/tvstaticparams.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestTVStaticParams
 * @brief Contains static test methods for the TVStaticParams class.
 */
class TestTVStaticParams {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestResizeNoiseSpectrum();
    static void TestResizeScanSpectrum();
    static void TestNoiseSpectrumCount();
    static void TestScanSpectrumCount();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
