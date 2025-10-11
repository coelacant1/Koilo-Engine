/**
 * @file testimagematerial.hpp
 * @brief Unit tests for the ImageMaterial class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/implementations/imagematerial.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestImageMaterial
 * @brief Contains static test methods for the ImageMaterial class.
 */
class TestImageMaterial {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetHueAngle();
    static void TestUseUV();
    static void TestSetImage();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
