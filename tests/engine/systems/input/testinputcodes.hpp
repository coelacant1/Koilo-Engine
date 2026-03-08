// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testinputcodes.hpp
 * @brief Unit tests for the InputCodes class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/input/keycodes.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestInputCodes
 * @brief Contains static test methods for the InputCodes class.
 */
class TestInputCodes {
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
