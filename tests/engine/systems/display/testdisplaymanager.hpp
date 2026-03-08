// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testdisplaymanager.hpp
 * @brief Unit tests for the DisplayManager class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/display/displaymanager.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestDisplayManager
 * @brief Contains static test methods for the DisplayManager class.
 */
class TestDisplayManager {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    static void TestRemoveDisplay();
    static void TestGetDisplay();
    static void TestGetDisplayCount();
    static void TestGetDisplayIds();
    static void TestRouteCamera();
    static void TestUnrouteCamera();
    static void TestGetCameraDisplay();
    static void TestGetDisplayCamera();
    static void TestPresentAll();
    static void TestPresent();
    static void TestClearAll();
    static void TestClear();
    static void TestSetVSyncEnabled();
    static void TestSetAutoPresent();
    static void TestIsVSyncEnabled();
    static void TestIsAutoPresentEnabled();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestAddDisplayRaw();
    static void RunAllTests();
};
