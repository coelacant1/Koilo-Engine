/**
 * @file testphysicsmodule.hpp
 * @brief Unit tests for the PhysicsModule class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/physics/physics_module.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPhysicsModule
 * @brief Contains static test methods for the PhysicsModule class.
 */
class TestPhysicsModule {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetInfo();
    static void TestUpdate();
    static void TestShutdown();
    static void TestGetWorld();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
