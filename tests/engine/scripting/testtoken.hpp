// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testtoken.hpp
 * @brief Unit tests for the Token class.
 *
 * @date 15/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/koiloscript_lexer.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestToken
 * @brief Contains static test methods for the Token class.
 */
class TestToken {
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
