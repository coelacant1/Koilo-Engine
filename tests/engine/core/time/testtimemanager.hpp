// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testtimemanager.hpp
 * @brief Unit tests for the TimeManager class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/time/timemanager.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestTimeManager
 * @brief Contains static test methods for the TimeManager class.
 */
class TestTimeManager {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetDeltaTime();
    static void TestGetTotalTime();
    static void TestGetFrameCount();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
