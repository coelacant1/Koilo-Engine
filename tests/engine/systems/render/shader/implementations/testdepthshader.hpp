/**
 * @file testdepthshader.hpp
 * @brief Unit tests for the DepthShader class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/shader/implementations/depthshader.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestDepthShader
 * @brief Contains static test methods for the DepthShader class.
 */
class TestDepthShader {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestShade();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
