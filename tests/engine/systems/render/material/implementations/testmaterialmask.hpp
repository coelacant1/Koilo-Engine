/**
 * @file testmaterialmask.hpp
 * @brief Unit tests for the MaterialMask class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/implementations/materialmask.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMaterialMask
 * @brief Contains static test methods for the MaterialMask class.
 */
class TestMaterialMask {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetOpacity();
    static void TestGetOpacityReference();
    static void TestSetShape();
    static void TestSetMaterialShape();
    static void TestSetMaterialOuter();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
