/**
 * @file testspectrumanalyzerparams.hpp
 * @brief Unit tests for the SpectrumAnalyzerParams class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/shader/implementations/spectrumanalyzerparams.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSpectrumAnalyzerParams
 * @brief Contains static test methods for the SpectrumAnalyzerParams class.
 */
class TestSpectrumAnalyzerParams {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestResizeSpectrum();
    static void TestResizeBins();
    static void TestResize();
    static void TestSpectrumCount();
    static void TestBinCount();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
