/**
 * @file testconsolesocket.hpp
 * @brief Unit tests for the ConsoleSocket class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/console/console_socket.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestConsoleSocket
 * @brief Contains static test methods for the ConsoleSocket class.
 */
class TestConsoleSocket {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestIsRunning();
    static void TestPort();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
