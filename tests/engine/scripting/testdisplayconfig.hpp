// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testdisplayconfig.hpp
 * @brief Unit tests for the DisplayConfig class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/display_config.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestDisplayConfig
 * @brief Contains static test methods for the DisplayConfig class.
 */
class TestDisplayConfig {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetType();
    static void TestSetWidth();
    static void TestSetHeight();
    static void TestSetPixelWidth();
    static void TestSetPixelHeight();
    static void TestSetBrightness();
    static void TestSetTargetFPS();
    static void TestGetType();
    static void TestGetWidth();
    static void TestGetHeight();
    static void TestGetPixelWidth();
    static void TestGetPixelHeight();
    static void TestGetBrightness();
    static void TestGetTargetFPS();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestGetCapFPS();
    static void TestSetCapFPS();
    static void RunAllTests();
};
