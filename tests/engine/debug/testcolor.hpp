// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcolor.hpp
 * @brief Unit tests for the Color class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/debug/debugdraw.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestColor
 * @brief Contains static test methods for the Color class.
 */
class TestColor {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
