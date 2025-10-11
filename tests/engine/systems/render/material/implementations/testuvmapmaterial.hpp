/**
 * @file testuvmapmaterial.hpp
 * @brief Unit tests for the UVMapMaterial class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/implementations/uvmapmaterial.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestUVMapMaterial
 * @brief Contains static test methods for the UVMapMaterial class.
 */
class TestUVMapMaterial {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetHueAngle();
    static void TestSetFlipU();
    static void TestSetFlipV();
    static void TestSetSize();
    static void TestSetImage();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
