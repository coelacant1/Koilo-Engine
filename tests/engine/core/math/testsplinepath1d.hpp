/**
 * @file testsplinepath1d.hpp
 * @brief Unit tests for the SplinePath1D class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/math/spline_path_1d.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSplinePath1D
 * @brief Contains static test methods for the SplinePath1D class.
 */
class TestSplinePath1D {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestAddPoint();
    static void TestClear();
    static void TestGetPointCount();
    static void TestSetLooping();
    static void TestGetLooping();
    static void TestSetAngleMode();
    static void TestGetPoint();
    static void TestEvaluate();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
