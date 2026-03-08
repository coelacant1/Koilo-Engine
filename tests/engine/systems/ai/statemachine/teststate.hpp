// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file teststate.hpp
 * @brief Unit tests for the State class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ai/statemachine/statemachine.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestState
 * @brief Contains static test methods for the State class.
 */
class TestState {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetName();
    static void TestEnter();
    static void TestUpdate();
    static void TestExit();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
