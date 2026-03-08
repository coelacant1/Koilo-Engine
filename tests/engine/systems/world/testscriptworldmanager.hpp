// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscriptworldmanager.hpp
 * @brief Unit tests for the ScriptWorldManager class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/world/script_world_manager.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestScriptWorldManager
 * @brief Contains static test methods for the ScriptWorldManager class.
 */
class TestScriptWorldManager {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestCreateLevel();
    static void TestRemoveLevel();
    static void TestGetLevelCount();
    static void TestSetActiveLevel();
    static void TestGetActiveLevelName();
    static void TestLoadLevel();
    static void TestUnloadLevel();
    static void TestUnloadAllInactive();
    static void TestSaveLevel();
    static void TestLoadLevelFromFile();
    static void TestSetStreamingEnabled();
    static void TestIsStreamingEnabled();
    static void TestSetViewerPosition();
    static void TestSetStreamingInterval();
    static void TestCheckStreaming();
    static void TestUpdate();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
