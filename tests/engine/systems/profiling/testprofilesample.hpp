// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testprofilesample.hpp
 * @brief Unit tests for the ProfileSample class.
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
 * @class TestProfileSample
 * @brief Contains static test methods for the ProfileSample class.
 */
class TestProfileSample {
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
