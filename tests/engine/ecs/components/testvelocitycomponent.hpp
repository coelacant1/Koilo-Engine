// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testvelocitycomponent.hpp
 * @brief Unit tests for the VelocityComponent class.
 *
 * @date 16/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/ecs/components/velocitycomponent.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestVelocityComponent
 * @brief Contains static test methods for the VelocityComponent class.
 */
class TestVelocityComponent {
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
