// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscriptcontext.hpp
 * @brief Unit tests for the ScriptContext class.
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
 * @class TestScriptContext
 * @brief Contains static test methods for the ScriptContext class.
 */
class TestScriptContext {
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
