// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testellipse.hpp
 * @brief Unit tests for the Ellipse2D source file (ellipse.cpp).
 *
 * @date 24/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/geometry/2d/ellipse.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestEllipse
 * @brief Contains static test methods for ellipse.cpp implementation.
 */
class TestEllipse {
public:
    // Constructor tests
    static void TestConstructorWithCenterSizeRotation();
    static void TestConstructorWithBounds();

    // Method tests
    static void TestIsInShape();
    static void TestIsInShapeWithRotation();

    // Edge case tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
