/**
 * @file testproceduralnoiseparams.hpp
 * @brief Unit tests for the ProceduralNoiseParams class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/shader/implementations/proceduralnoiseparams.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestProceduralNoiseParams
 * @brief Contains static test methods for the ProceduralNoiseParams class.
 */
class TestProceduralNoiseParams {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestResizeSpectrum();
    static void TestSpectrumCount();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
