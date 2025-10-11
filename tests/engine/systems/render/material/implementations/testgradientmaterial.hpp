/**
 * @file testgradientmaterial.hpp
 * @brief Unit tests for the GradientMaterial class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/implementations/gradientmaterial.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestGradientMaterial
 * @brief Contains static test methods for the GradientMaterial class.
 */
class TestGradientMaterial {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetPositionOffset();
    static void TestSetRotationOffset();
    static void TestSetRotationAngle();
    static void TestSetGradientPeriod();
    static void TestGradientShift();
    static void TestSetColorCount();
    static void TestColorCount();
    static void TestUpdateGradient();
    static void TestSetSpectrumAt();
    static void TestGetSpectrumAt();
    // ... add tests for remaining 4 methods

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestHueShift();
    static void TestSetBasePalette();
    static void TestUpdateRGB();
    static void RunAllTests();
};
