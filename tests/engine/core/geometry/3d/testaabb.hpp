// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testaabb.hpp
 * @brief Unit tests for the AABB class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/geometry/3d/aabb.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAABB
 * @brief Contains static test methods for the AABB class.
 */
class TestAABB {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetCenter();
    static void TestGetSize();
    static void TestGetHalfSize();
    static void TestGetVolume();
    static void TestContains();
    static void TestOverlaps();
    static void TestUnion();
    static void TestEncapsulate();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
