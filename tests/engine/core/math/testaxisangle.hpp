/**
 * @file testaxisangle.hpp
 * @brief Unit tests for the AxisAngle class.
 *
 * Tests for rotation representation using axis vector and angle.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/math/axisangle.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAxisAngle
 * @brief Contains static test methods for the AxisAngle class.
 */
class TestAxisAngle {
public:
    // Constructor tests

    // Field access tests

    // String conversion tests
    static void TestToString();

    // Axis vector tests

    // Edge case tests

    /**
     * @brief Runs all test methods.
     */
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestParameterizedConstructor();
    static void RunAllTests();
};
