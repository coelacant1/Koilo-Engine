/**
 * @file testconsoleresult.hpp
 * @brief Unit tests for the ConsoleResult class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/console/console_result.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestConsoleResult
 * @brief Contains static test methods for the ConsoleResult class.
 */
class TestConsoleResult {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void Testok();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
