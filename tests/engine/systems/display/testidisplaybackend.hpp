// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testidisplaybackend.hpp
 * @brief Unit tests for the IDisplayBackend class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/display/idisplaybackend.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestIDisplayBackend
 * @brief Contains static test methods for the IDisplayBackend class.
 */
class TestIDisplayBackend {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestInitialize();
    static void TestShutdown();
    static void TestIsInitialized();
    static void TestGetInfo();
    static void TestHasCapability();
    static void TestPresent();
    static void TestWaitVSync();
    static void TestClear();
    static void TestSetRefreshRate();
    static void TestSetOrientation();
    static void TestSetBrightness();
    static void TestSetVSyncEnabled();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
