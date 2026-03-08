// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmemorystats.hpp
 * @brief Unit tests for the MemoryStats class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/profiling/memoryprofiler.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMemoryStats
 * @brief Contains static test methods for the MemoryStats class.
 */
class TestMemoryStats {
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
