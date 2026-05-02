/**
 * @file testmessagebus.hpp
 * @brief Unit tests for the MessageBus class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/service_registry.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestMessageBus
 * @brief Contains static test methods for the MessageBus class.
 */
class TestMessageBus {
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
