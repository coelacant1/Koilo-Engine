/**
 * @file testaerodynamicsworld.hpp
 * @brief Unit tests for the AerodynamicsWorld class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/aerodynamics/aerodynamicsworld.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAerodynamicsWorld
 * @brief Contains static test methods for the AerodynamicsWorld class.
 */
class TestAerodynamicsWorld {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestAttachToPhysics();
    static void TestDetachFromPhysics();
    static void TestSetWindField();
    static void TestGetWindField();
    static void TestSetWorldUp();
    static void TestGetWorldUp();
    static void TestRegisterSurface();
    static void TestRegisterEngine();
    static void TestUnregisterSurface();
    static void TestUnregisterEngine();
    static void TestClear();
    static void TestGetSurfaceCount();
    static void TestGetEngineCount();
    static void TestGetSimTime();
    static void TestResetSimTime();
    static void TestStep();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
