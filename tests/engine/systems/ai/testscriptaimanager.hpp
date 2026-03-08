// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscriptaimanager.hpp
 * @brief Unit tests for the ScriptAIManager class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ai/script_ai_manager.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestScriptAIManager
 * @brief Contains static test methods for the ScriptAIManager class.
 */
class TestScriptAIManager {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
