// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testkoiloscriptlexer.hpp
 * @brief Unit tests for the KoiloScriptLexer class.
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
 * @class TestKoiloScriptLexer
 * @brief Contains static test methods for the KoiloScriptLexer class.
 */
class TestKoiloScriptLexer {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestTokenize();
    static void TestHasError();
    static void TestGetError();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
