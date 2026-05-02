/**
 * @file testconsolemodule.hpp
 * @brief Unit tests for the ConsoleModule class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/console/console_module.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestConsoleModule
 * @brief Contains static test methods for the ConsoleModule class.
 */
class TestConsoleModule {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestExecute();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
