// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcolorconverter.hpp
 * @brief Unit tests for the ColorConverter class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/color/colorconverter.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestColorConverter
 * @brief Contains static test methods for the ColorConverter class.
 */
class TestColorConverter {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestWritePixel();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
