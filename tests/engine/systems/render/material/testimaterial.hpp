// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testimaterial.hpp
 * @brief Unit tests for the IMaterial class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/material/imaterial.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestIMaterial
 * @brief Contains static test methods for the IMaterial class.
 */
class TestIMaterial {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetShader();
    static void TestUpdate();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
