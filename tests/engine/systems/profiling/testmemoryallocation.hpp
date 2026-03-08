// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmemoryallocation.hpp
 * @brief Unit tests for the MemoryAllocation class.
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
 * @class TestMemoryAllocation
 * @brief Contains static test methods for the MemoryAllocation class.
 */
class TestMemoryAllocation {
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
