// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcolorpalette.hpp
 * @brief Unit tests for the ColorPalette class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/color/colorpalette.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestColorPalette
 * @brief Contains static test methods for the ColorPalette class.
 */
class TestColorPalette {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestAdd();
    static void TestAddColor();
    static void TestSetAt();
    static void TestGetAt();
    static void TestGetCount();
    static void TestClear();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
