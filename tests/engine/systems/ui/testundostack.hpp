// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testundostack.hpp
 * @brief Test declarations for undo/redo command stack.
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ui/undo_stack.hpp>
#include <koilo/registry/registry.hpp>
#include <koilo/registry/reflect_macros.hpp>

class TestUndoStack {
public:
    static void TestBasicPushUndoRedo();
    static void TestCanUndoRedoStates();
    static void TestRedoClearedOnPush();
    static void TestMultipleUndos();
    static void TestCommandNames();
    static void TestMaxDepth();
    static void TestClear();
    static void TestOnChangeCallback();
    static void TestPropertyChangeCommand();
    static void TestPropertyChangeName();
    static void TestCursorTracking();

    static void RunAllTests();
};

