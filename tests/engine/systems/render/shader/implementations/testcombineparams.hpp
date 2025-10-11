/**
 * @file testcombineparams.hpp
 * @brief Unit tests for the CombineParams class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/shader/implementations/combineparams.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCombineParams
 * @brief Contains static test methods for the CombineParams class.
 */
class TestCombineParams {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestAddLayer();
    static void TestSetMethod();
    static void TestSetOpacity();
    static void TestSetMaterial();
    static void TestClear();
    static void TestReserve();
    static void TestLayerCount();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
