// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testui.hpp
 * @brief Unit tests for the UI class.
 *
 * @date 22/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ui/ui.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestUI
 * @brief Contains static test methods for the UI class.
 */
class TestUI {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestCreateWidget();
    static void TestCreateLabel();
    static void TestCreatePanel();
    static void TestCreateButton();
    static void TestGetWidgetCount();
    static void TestGetWidget();
    static void TestRebuildFocusList();
    static void TestNextFocus();
    static void TestPrevFocus();
    static void TestActivateFocus();
    static void TestGetFocusedWidget();

    static void TestClear();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestGetFocusCount();
    static void TestHitTest();
    static void RunAllTests();
};
