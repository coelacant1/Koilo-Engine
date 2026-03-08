// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsystem.hpp
 * @brief Unit tests for the System class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/ecs/system.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSystem
 * @brief Contains static test methods for the System class.
 */
class TestSystem {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetEnabled();
    static void TestIsEnabled();
    static void TestSetPriority();
    static void TestGetPriority();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
