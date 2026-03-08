// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testreflectionbridge.hpp
 * @brief Unit tests for the ReflectionBridge class.
 *
 * @date 15/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/reflection_bridge.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestReflectionBridge
 * @brief Contains static test methods for the ReflectionBridge class.
 */
class TestReflectionBridge {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetFieldPointer();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestInvokeMethod();
    static void RunAllTests();
};
