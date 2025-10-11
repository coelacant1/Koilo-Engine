/**
 * @file testvectorfield2dparams.hpp
 * @brief Unit tests for the VectorField2DParams class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/shader/implementations/vectorfield2dparams.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestVectorField2DParams
 * @brief Contains static test methods for the VectorField2DParams class.
 */
class TestVectorField2DParams {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
