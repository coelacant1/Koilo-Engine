/**
 * @file testquadbezier.hpp
 * @brief Unit tests for the QuadBezier class.
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
 * @class TestQuadBezier
 * @brief Contains static test methods for the QuadBezier class.
 */
class TestQuadBezier {
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
