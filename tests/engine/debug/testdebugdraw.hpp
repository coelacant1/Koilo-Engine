// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testdebugdraw.hpp
 * @brief Unit tests for the DebugDraw class.
 *
 * @date 11/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/debug/debugdraw.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestDebugDraw
 * @brief Contains static test methods for the DebugDraw class.
 */
class TestDebugDraw {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestEnable();
    static void TestDisable();
    static void TestIsEnabled();
    static void TestUpdate();
    static void TestClear();
    static void TestDrawLine();
    static void TestDrawSphere();
    static void TestDrawBox();
    static void TestDrawAxes();
    static void TestDrawGrid();
    static void TestDrawText();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestDrawOrientedBox();
    static void RunAllTests();
};
