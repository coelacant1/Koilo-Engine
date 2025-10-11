/**
 * @file testeulerangles.hpp
 * @brief Unit tests for the EulerAngles class.
 *
 * Tests for Euler angles representation and rotation order.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/math/eulerangles.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestEulerAngles
 * @brief Contains static test methods for the EulerAngles class.
 */
class TestEulerAngles {
public:
    // Constructor tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Field access tests

    // String conversion tests
    static void TestToString();

    // Rotation order tests

    // Edge case tests

    /**
     * @brief Runs all test methods.
     */
    static void TestEdgeCases();
    static void RunAllTests();
};
