// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testprofileresult.hpp
 * @brief Unit tests for the ProfileResult class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/debug/profiler.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestProfileResult
 * @brief Contains static test methods for the ProfileResult class.
 */
class TestProfileResult {
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
