/**
 * @file testuniformcolormaterial.hpp
 * @brief Unit tests for the UniformColorMaterial class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/implementations/uniformcolormaterial.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestUniformColorMaterial
 * @brief Contains static test methods for the UniformColorMaterial class.
 */
class TestUniformColorMaterial {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestHueShift();
    static void TestSetRGB();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
