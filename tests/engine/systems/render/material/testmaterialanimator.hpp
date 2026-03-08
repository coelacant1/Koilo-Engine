// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmaterialanimator.hpp
 * @brief Unit tests for the MaterialAnimator class.
 *
 * @date 16/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/material/materialanimator.hpp>
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

    static void TestAddMaterialFrame();
    static void TestGetMaterialOpacity();
    static void TestGetCapacity();
    static void TestGetActiveLayerCount();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */

    static void RunAllTests();
};
