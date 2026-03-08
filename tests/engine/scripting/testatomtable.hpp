// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testatomtable.hpp
 * @brief Unit tests for the AtomTable class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/string_atom.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAtomTable
 * @brief Contains static test methods for the AtomTable class.
 */
class TestAtomTable {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestIntern();
    static void TestGetString();
    static void TestContains();
    static void TestFind();
    static void TestSize();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
