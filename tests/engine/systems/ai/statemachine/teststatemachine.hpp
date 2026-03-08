// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file teststatemachine.hpp
 * @brief Unit tests for the StateMachine class.
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
 * @class TestStateMachine
 * @brief Contains static test methods for the StateMachine class.
 */
class TestStateMachine {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetCurrentStateName();
    static void TestSetInitialState();
    static void TestTransitionTo();
    static void TestStart();
    static void TestStop();
    static void TestUpdate();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
