/**
 * @file testconstantwind.hpp
 * @brief Unit tests for the ConstantWind class.
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
 * @class TestConstantWind
 * @brief Contains static test methods for the ConstantWind class.
 */
class TestConstantWind {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetVelocity();
    static void TestGetVelocity();
    static void TestSample();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
