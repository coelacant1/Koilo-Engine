/**
 * @file testconfig.hpp
 * @brief Unit tests for the Config class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/kernel.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestConfig
 * @brief Contains static test methods for the Config class.
 */
class TestConfig {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
