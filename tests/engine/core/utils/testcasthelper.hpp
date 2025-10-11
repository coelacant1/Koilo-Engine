/**
 * @file testcasthelper.hpp
 * @brief Unit tests for the CastHelper class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/utils/casthelper.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCastHelper
 * @brief Contains static test methods for the CastHelper class.
 */
class TestCastHelper {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestToU16();
    static void TestToI32();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
