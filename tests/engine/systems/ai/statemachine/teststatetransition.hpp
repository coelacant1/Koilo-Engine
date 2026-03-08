// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file teststatetransition.hpp
 * @brief Unit tests for the StateTransition class.
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
 * @class TestStateTransition
 * @brief Contains static test methods for the StateTransition class.
 */
class TestStateTransition {
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
