/**
 * @file testrasterizer.hpp
 * @brief Unit tests for the Rasterizer class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/raster/rasterizer.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestRasterizer
 * @brief Contains static test methods for the Rasterizer class.
 */
class TestRasterizer {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
