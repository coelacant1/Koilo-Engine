// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscriptentitymanager.hpp
 * @brief Unit tests for the ScriptEntityManager class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ecs/script_entity_manager.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestScriptEntityManager
 * @brief Contains static test methods for the ScriptEntityManager class.
 */
class TestScriptEntityManager {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    static void TestGetCount();

    static void TestSyncTransforms();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
