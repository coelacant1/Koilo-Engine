// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testpathfinder.cpp
 * @brief Implementation of Pathfinding tests.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testpathfinder.hpp"
#include <koilo/systems/ai/pathfinding/pathfinder.hpp>
#include <cmath>

using namespace koilo;

// === GridNode Tests ===

void TestPathfinder::TestGridNodeConstruction() {
    GridNode node1;
    TEST_ASSERT_EQUAL(0, node1.x);
    TEST_ASSERT_EQUAL(0, node1.y);
    TEST_ASSERT_TRUE(node1.walkable);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, node1.cost);
    
    GridNode node2(5, 10, false, 2.5f);
    TEST_ASSERT_EQUAL(5, node2.x);
    TEST_ASSERT_EQUAL(10, node2.y);
    TEST_ASSERT_FALSE(node2.walkable);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.5f, node2.cost);
}

void TestPathfinder::TestGridNodeProperties() {
    GridNode node(3, 7);
    
    node.walkable = false;
    TEST_ASSERT_FALSE(node.walkable);
    
    node.cost = 3.5f;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.5f, node.cost);
}

void TestPathfinder::TestGridNodeEquality() {
    GridNode node1(5, 10);
    GridNode node2(5, 10);
    GridNode node3(6, 10);
    
    TEST_ASSERT_TRUE(node1 == node2);
    TEST_ASSERT_FALSE(node1 == node3);
}

// === PathfinderGrid Tests ===

void TestPathfinder::TestPathfinderConstruction() {
    PathfinderGrid grid(10, 15);
    
    // Should be able to access nodes within bounds
    TEST_ASSERT_TRUE(grid.IsInBounds(0, 0));
    TEST_ASSERT_TRUE(grid.IsInBounds(9, 14));
}

void TestPathfinder::TestBoundsChecking() {
    PathfinderGrid grid(10, 10);
    
    // Valid bounds
    TEST_ASSERT_TRUE(grid.IsInBounds(0, 0));
    TEST_ASSERT_TRUE(grid.IsInBounds(5, 5));
    TEST_ASSERT_TRUE(grid.IsInBounds(9, 9));
    
    // Invalid bounds
    TEST_ASSERT_FALSE(grid.IsInBounds(-1, 0));
    TEST_ASSERT_FALSE(grid.IsInBounds(0, -1));
    TEST_ASSERT_FALSE(grid.IsInBounds(10, 0));
    TEST_ASSERT_FALSE(grid.IsInBounds(0, 10));
    TEST_ASSERT_FALSE(grid.IsInBounds(10, 10));
}

void TestPathfinder::TestSetWalkable() {
    PathfinderGrid grid(10, 10);
    
    grid.SetWalkable(5, 5, false);
    
    const GridNode* node = grid.GetNode(5, 5);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(node->walkable);
}

void TestPathfinder::TestSetCost() {
    PathfinderGrid grid(10, 10);
    
    grid.SetCost(3, 4, 2.5f);
    
    const GridNode* node = grid.GetNode(3, 4);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.5f, node->cost);
}

void TestPathfinder::TestGetNode() {
    PathfinderGrid grid(10, 10);
    
    GridNode* node = grid.GetNode(5, 7);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(5, node->x);
    TEST_ASSERT_EQUAL(7, node->y);
    
    // Out of bounds
    GridNode* nullNode = grid.GetNode(-1, 0);
    TEST_ASSERT_NULL(nullNode);
}

void TestPathfinder::TestDiagonalMovement() {
    PathfinderGrid grid(10, 10, false);
    grid.SetAllowDiagonal(true);
    
    // Test by checking neighbors count (should be 8 in middle)
    const GridNode* node = grid.GetNode(5, 5);
    std::vector<GridNode*> neighbors = grid.GetNeighbors(node);
    
    // Should have 8 neighbors when diagonal is allowed
    TEST_ASSERT_TRUE(neighbors.size() > 4);
}

// === Pathfinding Tests ===

void TestPathfinder::TestStraightPath() {
    PathfinderGrid grid(10, 10);
    
    std::vector<GridNode> path;
    bool found = grid.FindPath(0, 0, 5, 0, path);
    
    TEST_ASSERT_TRUE(found);
    TEST_ASSERT_TRUE(path.size() > 0);
    
    // First node should be start
    TEST_ASSERT_EQUAL(0, path[0].x);
    TEST_ASSERT_EQUAL(0, path[0].y);
    
    // Last node should be goal
    TEST_ASSERT_EQUAL(5, path[path.size() - 1].x);
    TEST_ASSERT_EQUAL(0, path[path.size() - 1].y);
}

void TestPathfinder::TestDiagonalPath() {
    PathfinderGrid grid(10, 10, true); // Enable diagonal
    
    std::vector<GridNode> path;
    bool found = grid.FindPath(0, 0, 5, 5, path);
    
    TEST_ASSERT_TRUE(found);
    TEST_ASSERT_TRUE(path.size() > 0);
}

void TestPathfinder::TestObstaclePath() {
    PathfinderGrid grid(10, 10);
    
    // Create wall
    for (int y = 0; y < 5; y++) {
        grid.SetWalkable(5, y, false);
    }
    
    std::vector<GridNode> path;
    bool found = grid.FindPath(0, 2, 9, 2, path);
    
    TEST_ASSERT_TRUE(found);
    
    // Path should go around obstacle
    bool crossedObstacle = false;
    for (const auto& node : path) {
        if (node.x == 5 && node.y < 5) {
            crossedObstacle = true;
        }
    }
    TEST_ASSERT_FALSE(crossedObstacle);
}

void TestPathfinder::TestNoPath() {
    PathfinderGrid grid(10, 10);
    
    // Create complete wall
    for (int y = 0; y < 10; y++) {
        grid.SetWalkable(5, y, false);
    }
    
    std::vector<GridNode> path;
    bool found = grid.FindPath(0, 0, 9, 0, path);
    
    TEST_ASSERT_FALSE(found);
}

void TestPathfinder::TestStartEqualsGoal() {
    PathfinderGrid grid(10, 10);
    
    std::vector<GridNode> path;
    bool found = grid.FindPath(5, 5, 5, 5, path);
    
    // Should succeed with just the start/goal node
    TEST_ASSERT_TRUE(found);
    TEST_ASSERT_EQUAL(1, path.size());
    TEST_ASSERT_EQUAL(5, path[0].x);
    TEST_ASSERT_EQUAL(5, path[0].y);
}

void TestPathfinder::TestOutOfBounds() {
    PathfinderGrid grid(10, 10);
    
    std::vector<GridNode> path;
    
    // Start out of bounds
    bool found1 = grid.FindPath(-1, 0, 5, 5, path);
    TEST_ASSERT_FALSE(found1);
    
    // Goal out of bounds
    bool found2 = grid.FindPath(5, 5, 15, 15, path);
    TEST_ASSERT_FALSE(found2);
}

void TestPathfinder::TestCostCalculation() {
    PathfinderGrid grid(10, 10);
    
    // Set high cost area
    for (int x = 3; x <= 5; x++) {
        grid.SetCost(x, 5, 10.0f);
    }
    
    std::vector<GridNode> path;
    bool found = grid.FindPath(0, 5, 9, 5, path);
    
    TEST_ASSERT_TRUE(found);
    // Path should prefer to go around high cost area if possible
}

// === Neighbors Tests ===

void TestPathfinder::TestGetNeighbors4Way() {
    PathfinderGrid grid(10, 10, false); // No diagonal
    
    const GridNode* node = grid.GetNode(5, 5);
    std::vector<GridNode*> neighbors = grid.GetNeighbors(node);
    
    // Should have exactly 4 neighbors
    TEST_ASSERT_EQUAL(4, neighbors.size());
}

void TestPathfinder::TestGetNeighbors8Way() {
    PathfinderGrid grid(10, 10, true); // With diagonal
    
    const GridNode* node = grid.GetNode(5, 5);
    std::vector<GridNode*> neighbors = grid.GetNeighbors(node);
    
    // Should have exactly 8 neighbors
    TEST_ASSERT_EQUAL(8, neighbors.size());
}

void TestPathfinder::TestGetNeighborsBoundary() {
    PathfinderGrid grid(10, 10, true);
    
    // Corner node (0,0)
    const GridNode* corner = grid.GetNode(0, 0);
    std::vector<GridNode*> cornerNeighbors = grid.GetNeighbors(corner);
    TEST_ASSERT_EQUAL(3, cornerNeighbors.size()); // Only 3 neighbors
    
    // Edge node (5,0)
    const GridNode* edge = grid.GetNode(5, 0);
    std::vector<GridNode*> edgeNeighbors = grid.GetNeighbors(edge);
    TEST_ASSERT_EQUAL(5, edgeNeighbors.size()); // Only 5 neighbors
}

// === Heuristic Tests ===

void TestPathfinder::TestManhattanDistance() {
    GridNode a(0, 0);
    GridNode b(3, 4);
    
    float distance = PathfinderGrid::ManhattanDistance(a, b);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.0f, distance); // |3-0| + |4-0| = 7
}

void TestPathfinder::TestEuclideanDistance() {
    GridNode a(0, 0);
    GridNode b(3, 4);
    
    float distance = PathfinderGrid::EuclideanDistance(a, b);
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, distance); // sqrt(3^2 + 4^2) = 5
}

void TestPathfinder::TestDiagonalDistance() {
    GridNode a(0, 0);
    GridNode b(3, 4);
    
    float distance = PathfinderGrid::DiagonalDistance(a, b);
    
    // Chebyshev distance = max(|3-0|, |4-0|) = 4
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, distance);
}

void TestPathfinder::RunAllTests() {
    RUN_TEST(TestGridNodeConstruction);
    RUN_TEST(TestGridNodeProperties);
    RUN_TEST(TestGridNodeEquality);
    
    RUN_TEST(TestPathfinderConstruction);
    RUN_TEST(TestBoundsChecking);
    RUN_TEST(TestSetWalkable);
    RUN_TEST(TestSetCost);
    RUN_TEST(TestGetNode);
    RUN_TEST(TestDiagonalMovement);
    
    RUN_TEST(TestStraightPath);
    RUN_TEST(TestDiagonalPath);
    RUN_TEST(TestObstaclePath);
    RUN_TEST(TestNoPath);
    RUN_TEST(TestStartEqualsGoal);
    RUN_TEST(TestOutOfBounds);
    RUN_TEST(TestCostCalculation);
    
    RUN_TEST(TestGetNeighbors4Way);
    RUN_TEST(TestGetNeighbors8Way);
    RUN_TEST(TestGetNeighborsBoundary);
    
    RUN_TEST(TestManhattanDistance);
    RUN_TEST(TestEuclideanDistance);
    RUN_TEST(TestDiagonalDistance);
}
