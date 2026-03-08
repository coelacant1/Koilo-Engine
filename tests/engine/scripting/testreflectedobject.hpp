// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testreflectedobject.hpp
 * @brief Unit tests for the ReflectedObject class.
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
 * @class TestReflectedObject
 * @brief Contains static test methods for the ReflectedObject class.
 */
class TestReflectedObject {
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
