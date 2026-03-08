// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcomponentserializerentry.hpp
 * @brief Unit tests for the ComponentSerializerEntry class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/world/levelserializer.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestComponentSerializerEntry
 * @brief Contains static test methods for the ComponentSerializerEntry class.
 */
class TestComponentSerializerEntry {
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
