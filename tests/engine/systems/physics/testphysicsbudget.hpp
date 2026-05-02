/**
 * @file testphysicsbudget.hpp
 * @brief Unit tests for the PhysicsBudget class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/physics/physics_budget.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPhysicsBudget
 * @brief Contains static test methods for the PhysicsBudget class.
 */
class TestPhysicsBudget {
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
