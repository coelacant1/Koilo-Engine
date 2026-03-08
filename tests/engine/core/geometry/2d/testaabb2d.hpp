// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testaabb2d.hpp
 * @brief Unit tests for the AABB2D class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/geometry/2d/aabb2d.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAABB2D
 * @brief Contains static test methods for the AABB2D class.
 */
class TestAABB2D {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetCenter();
    static void TestGetSize();
    static void TestGetHalfSize();
    static void TestGetArea();
    static void TestContains();
    static void TestEncapsulate();
    static void TestClosestPoint();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
