/**
 * @file testtriangle2d.hpp
 * @brief Unit tests for the Triangle2D class.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela (Auto-Generated)
 */

#pragma once

#include <unity.h>
#include <ptx/core/geometry/2d/triangle.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestTriangle2D
 * @brief Contains static test methods for the Triangle2D class.
 */
class TestTriangle2D {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetArea();
    static void TestGetCentroid();
    static void TestIsInShape();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
