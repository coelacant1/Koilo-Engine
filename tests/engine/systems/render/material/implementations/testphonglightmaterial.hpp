/**
 * @file testphonglightmaterial.hpp
 * @brief Unit tests for the PhongLightMaterial class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/implementations/phonglightmaterial.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPhongLightMaterial
 * @brief Contains static test methods for the PhongLightMaterial class.
 */
class TestPhongLightMaterial {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetCameraPosition();
    static void TestSetAmbient();
    static void TestSetDiffuse();
    static void TestSetSpecular();
    static void TestSetShininess();
    static void TestLightAt();
    static void TestLightCount();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
