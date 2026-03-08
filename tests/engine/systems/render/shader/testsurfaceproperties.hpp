// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsurfaceproperties.hpp
 * @brief Unit tests for the SurfaceProperties class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/shader/ishader.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSurfaceProperties
 * @brief Contains static test methods for the SurfaceProperties class.
 */
class TestSurfaceProperties {
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
