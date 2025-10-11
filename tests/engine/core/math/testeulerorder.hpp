/**
 * @file testeulerorder.hpp
 * @brief Unit tests for the EulerOrder class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/math/eulerorder.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestEulerOrder
 * @brief Contains static test methods for the EulerOrder class.
 */
class TestEulerOrder {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestToString();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
