/**
 * @file testaerodynamicsurface.hpp
 * @brief Unit tests for the AerodynamicSurface class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/aerodynamics/aerodynamicsurface.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAerodynamicSurface
 * @brief Contains static test methods for the AerodynamicSurface class.
 */
class TestAerodynamicSurface {
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
