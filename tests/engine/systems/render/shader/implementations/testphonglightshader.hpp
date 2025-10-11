/**
 * @file testphonglightshader.hpp
 * @brief Unit tests for the PhongLightShader class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/shader/implementations/phonglightshader.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPhongLightShader
 * @brief Contains static test methods for the PhongLightShader class.
 */
class TestPhongLightShader {
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
