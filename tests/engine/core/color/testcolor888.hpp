// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcolor888.hpp
 * @brief Unit tests for the Color888 class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/color/color888.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestColor888
 * @brief Contains static test methods for the Color888 class.
 */
class TestColor888 {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSet();
    static void TestScale();
    static void TestAdd();
    static void TestHueShift();
    static void TestToString();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
