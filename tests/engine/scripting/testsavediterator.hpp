// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsavediterator.hpp
 * @brief Unit tests for the SavedIterator class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/coroutine.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSavedIterator
 * @brief Contains static test methods for the SavedIterator class.
 */
class TestSavedIterator {
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
