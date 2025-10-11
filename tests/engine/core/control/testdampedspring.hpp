/**
 * @file testdampedspring.hpp
 * @brief Unit tests for the DampedSpring class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/control/dampedspring.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestDampedSpring
 * @brief Contains static test methods for the DampedSpring class.
 */
class TestDampedSpring {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetCurrentPosition();
    static void TestSetConstants();
    static void TestCalculate();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
