// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcolormono.hpp
 * @brief Unit tests for the ColorMono class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/color/colormono.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestColorMono
 * @brief Contains static test methods for the ColorMono class.
 */
class TestColorMono {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSet();
    static void TestGet();
    static void TestTurnOn();
    static void TestTurnOff();
    static void TestToggle();
    static void TestToString();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
