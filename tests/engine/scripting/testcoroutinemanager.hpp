// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcoroutinemanager.hpp
 * @brief Unit tests for the CoroutineManager class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/coroutine.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCoroutineManager
 * @brief Contains static test methods for the CoroutineManager class.
 */
class TestCoroutineManager {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestStart();
    static void TestResumeAll();
    static void TestHasActive();
    static void TestCount();
    static void TestClear();
    static void TestPendingStarts();
    static void TestActive();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestStopByName();
    static void RunAllTests();
};
