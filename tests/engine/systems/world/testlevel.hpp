// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testlevel.hpp
 * @brief Unit tests for the Level class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/world/level.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestLevel
 * @brief Contains static test methods for the Level class.
 */
class TestLevel {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetName();
    static void TestSetName();
    static void TestGetEntityCount();
    static void TestIsStreamable();
    static void TestLoad();
    static void TestUnload();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
