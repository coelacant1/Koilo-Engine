/**
 * @file testaerodynamicsmodule.hpp
 * @brief Unit tests for the AerodynamicsModule class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/aerodynamics/aero_module.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAerodynamicsModule
 * @brief Contains static test methods for the AerodynamicsModule class.
 */
class TestAerodynamicsModule {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetInfo();
    static void TestUpdate();
    static void TestShutdown();
    static void TestGetWorld();
    static void TestGetDefaultWind();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
