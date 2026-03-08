// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testbytecodecompiler.hpp
 * @brief Unit tests for the BytecodeCompiler class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/bytecode_compiler.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestBytecodeCompiler
 * @brief Contains static test methods for the BytecodeCompiler class.
 */
class TestBytecodeCompiler {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestHasError();
    static void TestGetError();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
