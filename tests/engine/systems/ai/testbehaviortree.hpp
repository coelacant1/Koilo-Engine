// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testbehaviortree.hpp
 * @brief Unit tests for the BehaviorTree class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ai/behaviortree.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestBehaviorTree
 * @brief Contains static test methods for the BehaviorTree class.
 */
class TestBehaviorTree {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestStart();
    static void TestStop();
    static void TestIsRunning();
    static void TestGetName();
    static void TestSetName();
    static void TestReset();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
