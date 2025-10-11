/**
 * @file testmaterialanimatorparams.hpp
 * @brief Unit tests for the MaterialAnimatorParams class.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela (Auto-Generated)
 */

#pragma once

#include <unity.h>
#include <ptx/systems/render/material/materialanimator.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMaterialAnimatorParams
 * @brief Contains static test methods for the MaterialAnimatorParams class.
 */
class TestMaterialAnimatorParams {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
