// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscriptinstance.hpp
 * @brief Unit tests for the ScriptInstance class.
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
 * @class TestScriptInstance
 * @brief Contains static test methods for the ScriptInstance class.
 */
class TestScriptInstance {
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
