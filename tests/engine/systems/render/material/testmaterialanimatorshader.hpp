/**
 * @file testmaterialanimatorshader.hpp
 * @brief Unit tests for the MaterialAnimatorShader class.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela (Auto-Generated)
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/materialanimator.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMaterialAnimatorShader
 * @brief Contains static test methods for the MaterialAnimatorShader class.
 */
class TestMaterialAnimatorShader {
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
