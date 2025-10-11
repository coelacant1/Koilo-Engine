/**
 * @file testminfilter.hpp
 * @brief Unit tests for the MinFilter class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/filter/minfilter.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMinFilter
 * @brief Contains static test methods for the MinFilter class.
 */
class TestMinFilter {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestFilter();
    static void TestReset();
    static void TestGetCapacity();

    // Minimum tracking tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
