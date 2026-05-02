/**
 * @file testthrustengine.hpp
 * @brief Unit tests for the ThrustEngine class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/aerodynamics/thrustengine.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestThrustEngine
 * @brief Contains static test methods for the ThrustEngine class.
 */
class TestThrustEngine {
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
