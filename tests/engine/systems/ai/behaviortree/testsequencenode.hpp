// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsequencenode.hpp
 * @brief Unit tests for the SequenceNode class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ai/behaviortree/behaviortreenode.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSequenceNode
 * @brief Contains static test methods for the SequenceNode class.
 */
class TestSequenceNode {
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
