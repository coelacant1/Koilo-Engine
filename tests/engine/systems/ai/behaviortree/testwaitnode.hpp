// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testwaitnode.hpp
 * @brief Unit tests for the WaitNode class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ai/behaviortree/behaviortreeaction.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestWaitNode
 * @brief Contains static test methods for the WaitNode class.
 */
class TestWaitNode {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestUpdate();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
