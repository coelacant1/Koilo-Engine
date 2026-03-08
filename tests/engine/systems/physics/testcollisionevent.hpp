// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcollisionevent.hpp
 * @brief Unit tests for the CollisionEvent class.
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
 * @class TestCollisionEvent
 * @brief Contains static test methods for the CollisionEvent class.
 */
class TestCollisionEvent {
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
