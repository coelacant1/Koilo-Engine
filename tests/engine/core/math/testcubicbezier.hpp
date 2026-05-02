/**
 * @file testcubicbezier.hpp
 * @brief Unit tests for the CubicBezier class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/math/bezier.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCubicBezier
 * @brief Contains static test methods for the CubicBezier class.
 */
class TestCubicBezier {
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
