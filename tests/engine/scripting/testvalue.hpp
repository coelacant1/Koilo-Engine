// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testvalue.hpp
 * @brief Unit tests for the Value class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/script_context.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestValue
 * @brief Contains static test methods for the Value class.
 */
class TestValue {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    static void TestArrayNum();
    static void TestIsTruthy();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
