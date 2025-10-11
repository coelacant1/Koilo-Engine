/**
 * @file testhorizontalrainbowshader.hpp
 * @brief Unit tests for the HorizontalRainbowShader class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/shader/implementations/horizontalrainbowshader.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestHorizontalRainbowShader
 * @brief Contains static test methods for the HorizontalRainbowShader class.
 */
class TestHorizontalRainbowShader {
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
