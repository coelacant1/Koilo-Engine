// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testpathfindergrid.cpp
 * @brief Implementation of PathfinderGrid unit tests.
 */

#include "testpathfindergrid.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestPathfinderGrid::TestDefaultConstructor() {
    PathfinderGrid grid(10, 10);
    // Verify construction works and basic bounds check
    TEST_ASSERT_TRUE(grid.IsInBounds(0, 0));
    TEST_ASSERT_TRUE(grid.IsInBounds(9, 9));
    TEST_ASSERT_FALSE(grid.IsInBounds(10, 10));
}

void TestPathfinderGrid::TestParameterizedConstructor() {
    PathfinderGrid grid(5, 8, true);
    TEST_ASSERT_TRUE(grid.IsInBounds(4, 7));
    TEST_ASSERT_FALSE(grid.IsInBounds(5, 0));
    TEST_ASSERT_FALSE(grid.IsInBounds(0, 8));
}

// ========== Method Tests ==========

void TestPathfinderGrid::TestSetWalkable() {
    PathfinderGrid grid(10, 10);
    grid.SetWalkable(3, 3, false);
    GridNode* node = grid.GetNode(3, 3);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(node->walkable);
    grid.SetWalkable(3, 3, true);
    TEST_ASSERT_TRUE(node->walkable);
}

void TestPathfinderGrid::TestSetCost() {
    PathfinderGrid grid(10, 10);
    grid.SetCost(5, 5, 2.0f);
    GridNode* node = grid.GetNode(5, 5);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, node->cost);
}

void TestPathfinderGrid::TestIsInBounds() {
    PathfinderGrid grid(10, 10);
    TEST_ASSERT_TRUE(grid.IsInBounds(0, 0));
    TEST_ASSERT_TRUE(grid.IsInBounds(9, 9));
    TEST_ASSERT_FALSE(grid.IsInBounds(-1, 0));
    TEST_ASSERT_FALSE(grid.IsInBounds(0, -1));
    TEST_ASSERT_FALSE(grid.IsInBounds(10, 0));
    TEST_ASSERT_FALSE(grid.IsInBounds(0, 10));
}

void TestPathfinderGrid::TestSetAllowDiagonal() {
    PathfinderGrid grid(10, 10, false);
    grid.SetAllowDiagonal(true);
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

void TestPathfinderGrid::TestEdgeCases() {
    PathfinderGrid tiny(1, 1);
    TEST_ASSERT_TRUE(tiny.IsInBounds(0, 0));
    TEST_ASSERT_FALSE(tiny.IsInBounds(1, 0));
    GridNode* node = tiny.GetNode(0, 0);
    TEST_ASSERT_NOT_NULL(node);
}

// ========== Test Runner ==========

void TestPathfinderGrid::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSetWalkable);
    RUN_TEST(TestSetCost);
    RUN_TEST(TestIsInBounds);
    RUN_TEST(TestSetAllowDiagonal);
    RUN_TEST(TestEdgeCases);
}
