// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testgridnode.hpp
 * @brief Unit tests for the GridNode class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ai/pathfinding/pathfinder.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestGridNode
 * @brief Contains static test methods for the GridNode class.
 */
class TestGridNode {
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
