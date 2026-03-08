// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcircle.hpp
 * @brief Unit tests for the Circle2D source file (circle.cpp).
 *
 * @date 24/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/geometry/2d/circle.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCircle
 * @brief Contains static test methods for circle.cpp implementation.
 */
class TestCircle {
public:
    // Constructor tests
    static void TestConstructor();

    // Method tests
    static void TestIsInShape();

    // Edge case tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
