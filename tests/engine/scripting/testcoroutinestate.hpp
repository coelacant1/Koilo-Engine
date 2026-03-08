// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcoroutinestate.hpp
 * @brief Unit tests for the CoroutineState class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/bytecode_vm.hpp>
#include <koilo/scripting/coroutine.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCoroutineState
 * @brief Contains static test methods for the CoroutineState class.
 */
class TestCoroutineState {
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
