/**
 * @file testcorners.hpp
 * @brief Unit tests for the Corners class.
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
 * @class TestCorners
 * @brief Contains static test methods for the Corners class.
 */
class TestCorners {
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
