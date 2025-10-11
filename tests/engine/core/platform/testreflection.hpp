/**
 * @file testreflection.hpp
 * @brief Unit tests for the Reflection class.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela (Auto-Generated)
 */

#pragma once

#include <unity.h>
#include <ptx/core/platform/console.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestReflection
 * @brief Contains static test methods for the Reflection class.
 */
class TestReflection {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestBegin();
    static void TestPrint();
    static void TestPrintln();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
