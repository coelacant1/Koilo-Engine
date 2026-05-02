/**
 * @file testconsolesession.hpp
 * @brief Unit tests for the ConsoleSession class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/console/console_session.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestConsoleSession
 * @brief Contains static test methods for the ConsoleSession class.
 */
class TestConsoleSession {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetMaxHistory();
    static void TestSetAlias();
    static void TestRemoveAlias();
    static void TestAliases();
    static void TestSetOutputFormat();
    static void TestGetOutputFormat();
    static void TestFormatResult();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
