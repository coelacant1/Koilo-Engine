// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testvectorfield2d.hpp
 * @brief Unit tests for the VectorField2D class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/physics/vectorfield2d.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestVectorField2D
 * @brief Contains static test methods for the VectorField2D class.
 */
class TestVectorField2D {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    // ... add tests for remaining 5 methods

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */

    static void RunAllTests();
};
