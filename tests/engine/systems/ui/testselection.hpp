// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testselection.hpp
 * @brief Test declarations for selection manager.
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ui/selection.hpp>

class TestSelection {
public:
    static void TestInitiallyEmpty();
    static void TestSelectSingle();
    static void TestSelectReplaces();
    static void TestMultiSelect();
    static void TestAddDuplicateNoop();
    static void TestRemoveFromSelection();
    static void TestToggleSelection();
    static void TestClearSelection();
    static void TestPrimary();
    static void TestOnChangedCallback();
    static void TestClearEmptyNoCallback();

    static void RunAllTests();
};
