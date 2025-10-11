/**
 * @file testaudioreactiveparams.hpp
 * @brief Unit tests for the AudioReactiveParams class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/shader/implementations/audioreactiveparams.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAudioReactiveParams
 * @brief Contains static test methods for the AudioReactiveParams class.
 */
class TestAudioReactiveParams {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestResizeSpectrum();
    static void TestResizeSamples();
    static void TestSpectrumCount();
    static void TestSampleCount();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
