// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrayintersection.hpp
 * @brief Unit tests for the RayIntersection class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/ray/rayintersection.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRayIntersection
 * @brief Contains static test methods for the RayIntersection class.
 */
class TestRayIntersection {
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
