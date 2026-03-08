// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcolorgray.hpp
 * @brief Unit tests for the ColorGray class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/color/colorgray.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestColorGray
 * @brief Contains static test methods for the ColorGray class.
 */
class TestColorGray {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSet();
    static void TestGet();
    static void TestScale();
    static void TestAdd();
    static void TestToString();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
