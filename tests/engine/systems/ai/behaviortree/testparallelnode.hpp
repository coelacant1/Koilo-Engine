// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testparallelnode.hpp
 * @brief Unit tests for the ParallelNode class.
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
 * @class TestParallelNode
 * @brief Contains static test methods for the ParallelNode class.
 */
class TestParallelNode {
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
