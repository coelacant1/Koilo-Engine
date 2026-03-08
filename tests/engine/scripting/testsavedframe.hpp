// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsavedframe.hpp
 * @brief Unit tests for the SavedFrame class.
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
 * @class TestSavedFrame
 * @brief Contains static test methods for the SavedFrame class.
 */
class TestSavedFrame {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
