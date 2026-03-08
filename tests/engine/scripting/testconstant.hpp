// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testconstant.hpp
 * @brief Unit tests for the Constant class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/bytecode.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestConstant
 * @brief Contains static test methods for the Constant class.
 */
class TestConstant {
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
