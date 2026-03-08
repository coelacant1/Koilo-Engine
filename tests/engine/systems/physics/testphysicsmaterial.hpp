// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testphysicsmaterial.hpp
 * @brief Unit tests for the PhysicsMaterial class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/physics/physicsmaterial.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPhysicsMaterial
 * @brief Contains static test methods for the PhysicsMaterial class.
 */
class TestPhysicsMaterial {
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
