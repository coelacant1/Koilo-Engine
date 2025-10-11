/**
 * @file testrgbcolor.hpp
 * @brief Unit tests for the RGBColor class.
 *
 * Provides comprehensive tests for RGB color operations including
 * construction, arithmetic operations, color manipulation, and interpolation.
 *
 * @date 09/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/color/rgbcolor.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRGBColor
 * @brief Contains static test methods for the RGBColor class.
 */
class TestRGBColor {
public:
    // Constructor tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Setter tests
    static void TestSetColor();

    // Color manipulation tests
    static void TestScale();

    static void TestAdd();

    static void TestHueShift();

    // Static method tests

    // Operator tests

    // Utility tests
    static void TestToString();

    /**
     * @brief Runs all test methods.
     */
    static void TestEdgeCases();
    static void RunAllTests();
};
