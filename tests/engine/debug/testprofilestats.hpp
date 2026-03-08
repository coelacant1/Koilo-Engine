// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testprofilestats.hpp
 * @brief Unit tests for the ProfileStats class.
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
 * @class TestProfileStats
 * @brief Contains static test methods for the ProfileStats class.
 */
class TestProfileStats {
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
