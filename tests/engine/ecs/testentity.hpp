// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testentity.hpp
 * @brief Unit tests for the Entity class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/ecs/entity.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestEntity
 * @brief Contains static test methods for the Entity class.
 */
class TestEntity {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetID();
    static void TestGetIndex();
    static void TestGetGeneration();
    static void TestIsNull();
    static void TestIsValid();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
