/**
 * @file testautocvar_string.hpp
 * @brief Unit tests for the AutoCVar_String class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/cvar/cvar_system.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAutoCVar_String
 * @brief Contains static test methods for the AutoCVar_String class.
 */
class TestAutoCVar_String {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGet();
    static void TestSet();
    static void TestGetParam();
    static void TestOnChanged();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
