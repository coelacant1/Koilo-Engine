// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testeditorapp.hpp
 * @brief Tests for the editor application shell.
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ui/editor_app.hpp>

class TestEditorApp {
public:
    static void TestSetupCreatesRoot();
    static void TestMenuBarCreated();
    static void TestStatusBarCreated();
    static void TestDockAreaCreated();
    static void TestPanesCreated();
    static void TestHierarchyWidgets();
    static void TestInspectorWidgets();
    static void TestViewportWidgets();
    static void TestConsoleWidgets();
    static void TestWidgetCount();
    static void TestUpdateFps();
    static void TestSelectionInitial();
    static void TestUndoStackInitial();
    static void TestLogBufferInitial();

    static void RunAllTests();
};
