/**
 * @file testwait.hpp
 * @brief Unit tests for the Wait class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/time/wait.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestWait
 * @brief Contains static test methods for the Wait class.
 */
class TestWait {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestReset();
    static void TestIsFinished();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
