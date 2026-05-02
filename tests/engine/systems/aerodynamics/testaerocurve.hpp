/**
 * @file testaerocurve.hpp
 * @brief Unit tests for the AeroCurve class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/aerodynamics/aerocurve.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAeroCurve
 * @brief Contains static test methods for the AeroCurve class.
 */
class TestAeroCurve {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSample();
    static void TestSize();
    static void TestEmpty();
    static void TestInitFlatPlateLift();
    static void TestInitFlatPlateDrag();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
