// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcollisioninfo.hpp
 * @brief Unit tests for the CollisionInfo class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/physics/collisionmanager.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCollisionInfo
 * @brief Contains static test methods for the CollisionInfo class.
 */
class TestCollisionInfo {
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
