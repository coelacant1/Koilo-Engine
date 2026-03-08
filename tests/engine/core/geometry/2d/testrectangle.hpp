// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrectangle.hpp
 * @brief Unit tests for the Rectangle2D source file (rectangle.cpp).
 *
 * @date 24/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/geometry/2d/rectangle.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRectangle
 * @brief Contains static test methods for rectangle.cpp implementation.
 */
class TestRectangle {
public:
    // Constructor tests
    static void TestConstructorWithCenterSizeRotation();
    static void TestConstructorWithBounds();

    // Method tests
    static void TestIsInShape();
    static void TestIsInShapeWithRotation();
    static void TestGetCorners();
    static void TestUpdateBounds();
    static void TestGetters();
    static void TestOverlaps();
    static void TestContains();

    // Edge case tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
