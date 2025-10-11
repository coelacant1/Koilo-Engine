/**
 * @file testdepthmaterial.hpp
 * @brief Unit tests for the DepthMaterial class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/implementations/depthmaterial.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestDepthMaterial
 * @brief Contains static test methods for the DepthMaterial class.
 */
class TestDepthMaterial {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetAxis();
    static void TestSetDepth();
    static void TestSetOffset();
    static void TestGradient();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
