// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testtransformcomponent.hpp
 * @brief Unit tests for the TransformComponent class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/ecs/components/transformcomponent.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestTransformComponent
 * @brief Contains static test methods for the TransformComponent class.
 */
class TestTransformComponent {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetPosition();
    static void TestSetPosition();
    static void TestGetRotation();
    static void TestSetRotation();
    static void TestGetScale();
    static void TestSetScale();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
