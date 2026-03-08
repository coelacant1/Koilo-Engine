// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testnullbackend.hpp
 * @brief Unit tests for the NullBackend class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/display/backends/nullbackend.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestNullBackend
 * @brief Contains static test methods for the NullBackend class.
 */
class TestNullBackend {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestInitialize();
    static void TestShutdown();
    static void TestIsInitialized();
    static void TestGetInfo();
    static void TestPresent();
    static void TestClear();
    static void TestGetFrameCount();
    static void TestResetFrameCount();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestHasCapability();
    static void TestSetBrightness();
    static void TestSetOrientation();
    static void TestSetRefreshRate();
    static void TestSetVSyncEnabled();
    static void TestWaitVSync();
    static void RunAllTests();
};
