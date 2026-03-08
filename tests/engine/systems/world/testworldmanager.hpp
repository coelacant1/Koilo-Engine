// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testworldmanager.hpp
 * @brief Unit tests for the WorldManager class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/world/worldmanager.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestWorldManager
 * @brief Contains static test methods for the WorldManager class.
 */
class TestWorldManager {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetStreamingEnabled();
    static void TestIsStreamingEnabled();
    static void TestGetLevelCount();
    static void TestGetActiveLevelName();
    static void TestLoadLevel();
    static void TestUnloadLevel();
    static void TestUpdate();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
