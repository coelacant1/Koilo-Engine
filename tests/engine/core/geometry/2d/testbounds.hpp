// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testbounds.hpp
 * @brief Unit tests for the Bounds class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/core/geometry/2d/shape.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestBounds
 * @brief Contains static test methods for the Bounds class.
 */
class TestBounds {
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
