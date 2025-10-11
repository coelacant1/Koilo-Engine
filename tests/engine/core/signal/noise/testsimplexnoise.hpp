/**
 * @file testsimplexnoise.hpp
 * @brief Unit tests for the SimplexNoise class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/signal/noise/simplexnoise.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSimplexNoise
 * @brief Contains static test methods for the SimplexNoise class.
 */
class TestSimplexNoise {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    static void TestSetScale();
    static void TestSetZPosition();
    static void TestGetNoise();

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
