/**
 * @file testphonglightparams.hpp
 * @brief Unit tests for the PhongLightParams class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/shader/implementations/phonglightparams.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestPhongLightParams
 * @brief Contains static test methods for the PhongLightParams class.
 */
class TestPhongLightParams {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestResizeLights();
    static void TestLightCount();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
