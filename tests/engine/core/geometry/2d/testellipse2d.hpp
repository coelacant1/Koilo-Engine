/**
 * @file testellipse2d.hpp
 * @brief Unit tests for the Ellipse2D class.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela (Auto-Generated)
 */

#pragma once

#include <unity.h>
#include <ptx/core/geometry/2d/ellipse.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestEllipse2D
 * @brief Contains static test methods for the Ellipse2D class.
 */
class TestEllipse2D {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestIsInShape();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
