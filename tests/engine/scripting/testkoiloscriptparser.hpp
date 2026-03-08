// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testkoiloscriptparser.hpp
 * @brief Unit tests for the KoiloScriptParser class.
 *
 * @date 15/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/koiloscript_parser.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestKoiloScriptParser
 * @brief Contains static test methods for the KoiloScriptParser class.
 */
class TestKoiloScriptParser {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestParse();
    static void TestHasError();
    static void TestGetError();
    static void TestGetErrorLine();
    static void TestGetErrorColumn();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
