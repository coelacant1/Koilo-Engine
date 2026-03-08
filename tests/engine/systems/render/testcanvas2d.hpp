// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcanvas2d.hpp
 * @brief Unit tests for the Canvas2D class.
 *
 * @date 25/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/render/canvas2d.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestCanvas2D
 * @brief Contains static test methods for the Canvas2D class.
 */
class TestCanvas2D {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestResize();
    static void TestEnsureSize();
    static void TestClear();
    static void TestClearWithColor();
    static void TestSetPixel();
    static void TestFillRect();
    static void TestDrawRect();
    static void TestDrawLine();
    static void TestDrawCircle();
    static void TestFillCircle();
    static void TestGetWidth();
    static void TestGetHeight();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestAttach();
    static void TestDetach();
    static void TestDrawText();
    static void TestIsAttached();
    static void RunAllTests();
};
