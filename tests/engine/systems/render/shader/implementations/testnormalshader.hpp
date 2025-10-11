/**
 * @file testnormalshader.hpp
 * @brief Unit tests for the NormalShader class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/shader/implementations/normalshader.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestNormalShader
 * @brief Contains static test methods for the NormalShader class.
 *
 * Tests cover:
 * - Constructor validation
 * - Shade method with various normal directions
 * - Normal normalization behavior
 * - Edge cases (zero normals, negative components)
 * - Surface properties independence
 */
class TestNormalShader {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestShade();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
