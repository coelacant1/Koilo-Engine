// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmemoryscope.hpp
 * @brief Unit tests for the MemoryScope class.
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
 * @class TestMemoryScope
 * @brief Contains static test methods for the MemoryScope class.
 */
class TestMemoryScope {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestTrack();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
