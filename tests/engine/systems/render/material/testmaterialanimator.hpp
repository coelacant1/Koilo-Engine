/**
 * @file testmaterialanimator.hpp
 * @brief Unit tests for the MaterialAnimator class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/materialanimator.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMaterialAnimator
 * @brief Contains static test methods for the MaterialAnimator class.
 */
class TestMaterialAnimator {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetBaseMaterial();
    static void TestAddMaterial();
    static void TestAddMaterialFrame();
    static void TestGetMaterialOpacity();
    static void TestGetCapacity();
    static void TestGetActiveLayerCount();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
