// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsky.hpp
 * @brief Unit tests for the Sky class.
 *
 * @date 28/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/sky/sky.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSky
 * @brief Contains static test methods for the Sky class.
 */
class TestSky {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestEnable();
    static void TestDisable();
    static void TestIsEnabled();
    static void TestSetTimeOfDay();
    static void TestGetTimeOfDay();
    static void TestSetTimeSpeed();
    static void TestGetTimeSpeed();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
