/**
 * @file testphysicssimulator.hpp
 * @brief Unit tests for the PhysicsSimulator class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/physics/physicssimulator.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPhysicsSimulator
 * @brief Contains static test methods for the PhysicsSimulator class.
 */
class TestPhysicsSimulator {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetScene();
    static void TestUpdate();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
