/**
 * @file testfunctiongenerator.hpp
 * @brief Unit tests for the FunctionGenerator class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/functiongenerator.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestFunctionGenerator
 * @brief Contains static test methods for the FunctionGenerator class.
 */
class TestFunctionGenerator {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetPeriod();
    static void TestSetFunction();
    static void TestUpdate();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
