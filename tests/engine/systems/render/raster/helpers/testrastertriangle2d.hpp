/**
 * @file testrastertriangle2d.hpp
 * @brief Unit tests for the RasterTriangle2D class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/raster/helpers/rastertriangle2d.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRasterTriangle2D
 * @brief Contains static test methods for the RasterTriangle2D class.
 */
class TestRasterTriangle2D {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetBarycentricCoords();
    static void TestOverlaps();
    static void TestGetMaterial();
    static void TestToString();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
