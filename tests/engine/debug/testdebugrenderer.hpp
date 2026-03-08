// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testdebugrenderer.hpp
 * @brief Unit tests for the DebugRenderer class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/debug/debugrenderer.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestDebugRenderer
 * @brief Contains static test methods for the DebugRenderer class.
 */
class TestDebugRenderer {
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
