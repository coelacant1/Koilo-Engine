/**
 * @file testcombinematerial.hpp
 * @brief Unit tests for the CombineMaterial class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/implementations/combinematerial.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCombineMaterial
 * @brief Contains static test methods for the CombineMaterial class.
 */
class TestCombineMaterial {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestAddMaterial();
    static void TestSetMethod();
    static void TestSetOpacity();
    static void TestSetMaterial();
    static void TestClearLayers();
    static void TestReserveLayers();
    static void TestLayerCount();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
