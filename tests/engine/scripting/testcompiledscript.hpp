// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcompiledscript.hpp
 * @brief Unit tests for the CompiledScript class.
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
 * @class TestCompiledScript
 * @brief Contains static test methods for the CompiledScript class.
 */
class TestCompiledScript {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestFixupAtomPointers();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
