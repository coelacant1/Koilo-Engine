/**
 * @file testdirectionangle.hpp
 * @brief Unit tests for the DirectionAngle class.
 *
 * Tests for rotation representation using direction vector and angle.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/math/directionangle.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestDirectionAngle
 * @brief Contains static test methods for the DirectionAngle class.
 */
class TestDirectionAngle {
public:
    // Constructor tests

    // Field access tests

    // String conversion tests
    static void TestToString();

    // Direction vector tests

    // Edge case tests

    /**
     * @brief Runs all test methods.
     */
    static void TestDefaultConstructor();
    static void TestEdgeCases();
    static void TestParameterizedConstructor();
    static void RunAllTests();
};
