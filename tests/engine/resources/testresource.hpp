// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testresource.hpp
 * @brief Unit tests for the Resource class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/resources/resourcehandle.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestResource
 * @brief Contains static test methods for the Resource class.
 */
class TestResource {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetPath();
    static void TestIsLoaded();
    static void TestLoad();
    static void TestUnload();
    static void TestReload();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
