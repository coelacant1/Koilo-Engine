/**
 * @file testmaterialbinder.hpp
 * @brief Unit tests for the MaterialBinder class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/pipeline/material_binder.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMaterialBinder
 * @brief Contains static test methods for the MaterialBinder class.
 */
class TestMaterialBinder {
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
