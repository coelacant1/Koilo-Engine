// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcolor565.hpp
 * @brief Unit tests for the Color565 class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/color/color565.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestColor565
 * @brief Contains static test methods for the Color565 class.
 */
class TestColor565 {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSet();
    static void TestGetPacked();
    static void TestGetR5();
    static void TestGetG6();
    static void TestGetB5();
    static void TestGetR8();
    static void TestGetG8();
    static void TestGetB8();
    static void TestToString();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
