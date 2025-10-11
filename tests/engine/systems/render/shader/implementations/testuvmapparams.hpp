/**
 * @file testuvmapparams.hpp
 * @brief Unit tests for the UVMapParams class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/shader/implementations/uvmapparams.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestUVMapParams
 * @brief Contains static test methods for the UVMapParams class.
 */
class TestUVMapParams {
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
