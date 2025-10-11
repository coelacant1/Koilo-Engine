/**
 * @file testoscilloscopeparams.hpp
 * @brief Unit tests for the OscilloscopeParams class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/shader/implementations/oscilloscopeparams.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestOscilloscopeParams
 * @brief Contains static test methods for the OscilloscopeParams class.
 */
class TestOscilloscopeParams {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestResizeSpectrum();
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
