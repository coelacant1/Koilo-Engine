/**
 * @file testgradientcolor.hpp
 * @brief Unit tests for the GradientColor class.
 *
 * Tests for color gradient interpolation with smooth and stepped modes.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/color/gradientcolor.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestGradientColor
 * @brief Contains static test methods for the GradientColor class.
 */
class TestGradientColor {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // GetColorAt method tests
    static void TestGetColorAt();

    // Color management tests
    static void TestSetColors();

    static void TestGetColorCount();

    // Stepped mode tests
    static void TestIsStepped();
    static void TestSetStepped();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
