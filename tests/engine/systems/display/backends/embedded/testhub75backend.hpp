// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testhub75backend.hpp
 * @brief Unit tests for the HUB75Backend class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/display/backends/embedded/hub75backend.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestHUB75Backend
 * @brief Contains static test methods for the HUB75Backend class.
 */
class TestHUB75Backend {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
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
    static void TestGetColorDepth();
    static void TestSetGammaCorrectionEnabled();
    static void TestIsGammaCorrectionEnabled();
    static void TestSetScanPattern();
    static void TestGetConfig();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
