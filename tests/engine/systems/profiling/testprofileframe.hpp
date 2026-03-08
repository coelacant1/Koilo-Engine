// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testprofileframe.hpp
 * @brief Unit tests for the ProfileFrame class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/profiling/performanceprofiler.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestProfileFrame
 * @brief Contains static test methods for the ProfileFrame class.
 */
class TestProfileFrame {
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
