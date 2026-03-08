// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testpathfinder.hpp
 * @brief Unit tests for Pathfinding system.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestPathfinder {
public:
    // GridNode Tests
    static void TestGridNodeConstruction();
    static void TestGridNodeProperties();
    static void TestGridNodeEquality();
    
    // PathfinderGrid Tests
    static void TestPathfinderConstruction();
    static void TestBoundsChecking();
    static void TestSetWalkable();
    static void TestSetCost();
    static void TestGetNode();
    static void TestDiagonalMovement();
    
    // Pathfinding Tests
    static void TestStraightPath();
    static void TestDiagonalPath();
    static void TestObstaclePath();
    static void TestNoPath();
    static void TestStartEqualsGoal();
    static void TestOutOfBounds();
    static void TestCostCalculation();
    
    // Neighbors Tests
    static void TestGetNeighbors4Way();
    static void TestGetNeighbors8Way();
    static void TestGetNeighborsBoundary();
    
    // Heuristic Tests
    static void TestManhattanDistance();
    static void TestEuclideanDistance();
    static void TestDiagonalDistance();
    
    static void RunAllTests();
};

