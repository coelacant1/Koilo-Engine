// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testargmarshaller.hpp
 * @brief Unit tests for the ArgMarshaller class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/scripting/value_marshaller.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestArgMarshaller
 * @brief Contains static test methods for the ArgMarshaller class.
 */
class TestArgMarshaller {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
