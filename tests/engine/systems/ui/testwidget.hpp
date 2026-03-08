// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testwidget.hpp
 * @brief Unit tests for the Widget class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ui/widget.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestWidget
 * @brief Contains static test methods for the Widget class.
 */
class TestWidget {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestSetPosition();
    static void TestSetSize();
    static void TestSetVisible();
    static void TestSetEnabled();
    static void TestSetFocusable();
    static void TestSetText();
    static void TestSetTextColor();
    static void TestSetTextScale();
    static void TestSetBackgroundColor();
    static void TestSetBorderColor();
    static void TestSetBorderWidth();
    static void TestSetOnActivate();
    static void TestSetName();
    static void TestGetText();
    static void TestGetName();
    static void TestIsVisible();
    static void TestIsEnabled();
    static void TestIsFocused();
    static void TestAddChild();
    static void TestGetChildCount();
    static void TestContains();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
