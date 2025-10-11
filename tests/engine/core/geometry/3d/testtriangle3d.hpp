/**
 * @file testtriangle3d.hpp
 * @brief Unit tests for the Triangle3D class.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela (Auto-Generated)
 */

#pragma once

#include <unity.h>
#include <ptx/core/geometry/3d/triangle.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestTriangle3D
 * @brief Contains static test methods for the Triangle3D class.
 */
class TestTriangle3D {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetArea();
    static void TestGetNormal();
    static void TestGetCentroid();
    static void TestClosestPoint();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
