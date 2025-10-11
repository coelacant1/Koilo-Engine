/**
 * @file testoverlap2d.hpp
 * @brief Unit tests for the Overlap2D class.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela (Auto-Generated)
 */

#pragma once

#include <unity.h>
#include <ptx/core/geometry/2d/overlap.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestOverlap2D
 * @brief Contains static test methods for the Overlap2D class.
 */
class TestOverlap2D {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestOverlaps();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
