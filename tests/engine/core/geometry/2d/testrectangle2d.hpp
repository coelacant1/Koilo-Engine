/**
 * @file testrectangle2d.hpp
 * @brief Unit tests for the Rectangle2D class.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela (Auto-Generated)
 */

#pragma once

#include <unity.h>
#include <ptx/core/geometry/2d/rectangle.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRectangle2D
 * @brief Contains static test methods for the Rectangle2D class.
 */
class TestRectangle2D {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestIsInShape();
    static void TestGetCorners();
    static void TestUpdateBounds();
    static void TestGetMinimum();
    static void TestGetMaximum();
    static void TestGetCenter();
    static void TestContains();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
