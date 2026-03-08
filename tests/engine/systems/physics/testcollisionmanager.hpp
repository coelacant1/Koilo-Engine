// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcollisionmanager.hpp
 * @brief Unit tests for the CollisionManager class.
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
 * @class TestCollisionManager
 * @brief Contains static test methods for the CollisionManager class.
 */
class TestCollisionManager {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestRegisterCollider();
    static void TestUnregisterCollider();
    static void TestUnregisterAllColliders();
    static void TestSetLayerCollision();
    static void TestCanLayersCollide();
    static void TestSetDefaultCollisionMatrix();
    static void TestUpdate();
    static void TestClearCallbacks();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
