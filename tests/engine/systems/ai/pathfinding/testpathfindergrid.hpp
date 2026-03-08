// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testpathfindergrid.hpp
 * @brief Unit tests for the PathfinderGrid class.
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
 * @class TestPathfinderGrid
 * @brief Contains static test methods for the PathfinderGrid class.
 */
class TestPathfinderGrid {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetWalkable();
    static void TestSetCost();
    static void TestIsInBounds();
    static void TestSetAllowDiagonal();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
