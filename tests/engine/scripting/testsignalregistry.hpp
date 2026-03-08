// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsignalregistry.hpp
 * @brief Unit tests for the SignalRegistry class.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/signal_registry.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSignalRegistry
 * @brief Contains static test methods for the SignalRegistry class.
 */
class TestSignalRegistry {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestDeclareSignal();
    static void TestHasSignal();
    static void TestConnect();
    static void TestConnectOnce();
    static void TestDisconnect();
    static void TestGetHandlers();
    static void TestClear();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
