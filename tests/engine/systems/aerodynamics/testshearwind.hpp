/**
 * @file testshearwind.hpp
 * @brief Unit tests for the ShearWind class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/aerodynamics/windfield.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestShearWind
 * @brief Contains static test methods for the ShearWind class.
 */
class TestShearWind {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetBase();
    static void TestSetGradient();
    static void TestSample();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
