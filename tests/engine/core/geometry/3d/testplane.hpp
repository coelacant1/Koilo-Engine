/**
 * @file testplane.hpp
 * @brief Unit tests for the Plane class.
 *
 * This test suite validates the Plane class functionality including:
 * - Default constructor initialization
 * - Parameterized constructor with centroid and normal
 * - Public field access for Centroid and Normal
 * - ToString() method for string representation
 * - Edge cases with zero vectors, non-unit normals, and extreme values
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/geometry/3d/plane.hpp>
#include <ptx/core/platform/ustring.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPlane
 * @brief Contains static test methods for the Plane class.
 */
class TestPlane {
public:
    // Constructor tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Field access tests

    // Method tests
    static void TestToString();

    // Edge case tests

    /**
     * @brief Runs all test methods.
     */
    static void TestEdgeCases();
    static void RunAllTests();
};
