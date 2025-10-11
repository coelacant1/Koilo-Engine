/**
 * @file testtvstaticmaterial.hpp
 * @brief Unit tests for the TVStaticMaterial class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/implementations/tvstaticmaterial.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestTVStaticMaterial
 * @brief Contains static test methods for the TVStaticMaterial class.
 */
class TestTVStaticMaterial {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetNoiseGradientPeriod();
    static void TestSetNoiseHue();
    static void TestSetNoiseScale();
    static void TestSetScanGradientPeriod();
    static void TestSetScanMultiplyOpacity();
    static void TestSetScanShift();
    static void TestSetBarsCenter();
    static void TestSetBarsSize();
    static void TestSetBarsHue();
    static void TestSetNoiseSpectrumCount();
    // ... add tests for remaining 4 methods

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestNoiseSpectrumSize();
    static void TestScanSpectrumSize();
    static void TestSetScanSpectrumCount();
    static void TestUpdate();
    static void RunAllTests();
};
