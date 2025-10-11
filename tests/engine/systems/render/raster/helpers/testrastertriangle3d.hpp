/**
 * @file testrastertriangle3d.hpp
 * @brief Unit tests for the RasterTriangle3D class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/raster/helpers/rastertriangle3d.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRasterTriangle3D
 * @brief Contains static test methods for the RasterTriangle3D class.
 */
class TestRasterTriangle3D {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestIntersectsRay();
    static void TestGetNormal();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
