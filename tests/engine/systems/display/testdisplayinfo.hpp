// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testdisplayinfo.hpp
 * @brief Unit tests for the DisplayInfo class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/display/displayinfo.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestDisplayInfo
 * @brief Contains static test methods for the DisplayInfo class.
 */
class TestDisplayInfo {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestHasCapability();
    static void TestAddCapability();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
