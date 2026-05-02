// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcanvas2d.hpp
 * @brief Unit tests for the Canvas2D class.
 *
 * @date 25/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/canvas2d.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCanvas2D
 * @brief Contains static test methods for the Canvas2D class.
 */
class TestCanvas2D {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */

    static void RunAllTests();
};
