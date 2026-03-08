// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testreflectionserializer.hpp
 * @brief Unit tests for the ReflectionSerializer class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/world/reflectionserializer.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestReflectionSerializer
 * @brief Contains static test methods for the ReflectionSerializer class.
 */
class TestReflectionSerializer {
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
