// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testphysicsworld.hpp
 * @brief Unit tests for the PhysicsWorld class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/physics/physicsworld.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPhysicsWorld
 * @brief Contains static test methods for the PhysicsWorld class.
 */
class TestPhysicsWorld {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestAddBody();
    static void TestRemoveBody();
    static void TestRemoveAllBodies();
    static void TestGetBodyCount();
    static void TestGetGravity();
    static void TestSetGravity();
    static void TestSetFixedTimestep();
    static void TestSetMaxSubSteps();
    static void TestClearCollisionCallbacks();
    static void TestStep();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
