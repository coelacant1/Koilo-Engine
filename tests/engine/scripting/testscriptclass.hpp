// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscriptclass.hpp
 * @brief Unit tests for the ScriptClass class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/script_class.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestScriptClass
 * @brief Contains static test methods for the ScriptClass class.
 */
class TestScriptClass {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestFindField();
    static void TestHasMethod();
    static void RunAllTests();
};
