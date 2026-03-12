// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testselection.cpp
 * @brief Tests for the SelectionManager.
 * @date 03/09/2026
 * @author Coela
 */

#include "testselection.hpp"

static int s_objA = 1, s_objB = 2, s_objC = 3;

void TestSelection::TestInitiallyEmpty() {
    koilo::SelectionManager sel;
    TEST_ASSERT_TRUE(sel.IsEmpty());
    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(sel.Count()));
}

void TestSelection::TestSelectSingle() {
    koilo::SelectionManager sel;
    sel.Select(&s_objA, nullptr);
    TEST_ASSERT_EQUAL_UINT(1, static_cast<unsigned>(sel.Count()));
    TEST_ASSERT_TRUE(sel.IsSelected(&s_objA));
    TEST_ASSERT_TRUE(sel.IsSingle());
}

void TestSelection::TestSelectReplaces() {
    koilo::SelectionManager sel;
    sel.Select(&s_objA, nullptr);
    sel.Select(&s_objB, nullptr);
    TEST_ASSERT_EQUAL_UINT(1, static_cast<unsigned>(sel.Count()));
    TEST_ASSERT_FALSE(sel.IsSelected(&s_objA));
    TEST_ASSERT_TRUE(sel.IsSelected(&s_objB));
}

void TestSelection::TestMultiSelect() {
    koilo::SelectionManager sel;
    sel.Select(&s_objA, nullptr);
    sel.AddToSelection(&s_objB, nullptr);
    sel.AddToSelection(&s_objC, nullptr);
    TEST_ASSERT_EQUAL_UINT(3, static_cast<unsigned>(sel.Count()));
    TEST_ASSERT_TRUE(sel.IsSelected(&s_objA));
    TEST_ASSERT_TRUE(sel.IsSelected(&s_objB));
    TEST_ASSERT_TRUE(sel.IsSelected(&s_objC));
}

void TestSelection::TestAddDuplicateNoop() {
    koilo::SelectionManager sel;
    sel.Select(&s_objA, nullptr);
    sel.AddToSelection(&s_objA, nullptr);
    TEST_ASSERT_EQUAL_UINT(1, static_cast<unsigned>(sel.Count()));
}

void TestSelection::TestRemoveFromSelection() {
    koilo::SelectionManager sel;
    sel.Select(&s_objA, nullptr);
    sel.AddToSelection(&s_objB, nullptr);
    sel.RemoveFromSelection(&s_objA);
    TEST_ASSERT_EQUAL_UINT(1, static_cast<unsigned>(sel.Count()));
    TEST_ASSERT_FALSE(sel.IsSelected(&s_objA));
    TEST_ASSERT_TRUE(sel.IsSelected(&s_objB));
}

void TestSelection::TestToggleSelection() {
    koilo::SelectionManager sel;
    sel.ToggleSelection(&s_objA, nullptr);
    TEST_ASSERT_TRUE(sel.IsSelected(&s_objA));
    sel.ToggleSelection(&s_objA, nullptr);
    TEST_ASSERT_FALSE(sel.IsSelected(&s_objA));
}

void TestSelection::TestClearSelection() {
    koilo::SelectionManager sel;
    sel.Select(&s_objA, nullptr);
    sel.AddToSelection(&s_objB, nullptr);
    sel.ClearSelection();
    TEST_ASSERT_TRUE(sel.IsEmpty());
}

void TestSelection::TestPrimary() {
    koilo::SelectionManager sel;
    sel.Select(&s_objA, nullptr);
    sel.AddToSelection(&s_objB, nullptr);
    TEST_ASSERT_EQUAL_PTR(&s_objA, sel.Primary().instance);
}

void TestSelection::TestOnChangedCallback() {
    koilo::SelectionManager sel;
    int changeCount = 0;
    sel.SetOnChanged([&changeCount]() { changeCount++; });
    sel.Select(&s_objA, nullptr);
    sel.AddToSelection(&s_objB, nullptr);
    sel.RemoveFromSelection(&s_objA);
    sel.ClearSelection();
    TEST_ASSERT_EQUAL_INT(4, changeCount);
}

void TestSelection::TestClearEmptyNoCallback() {
    koilo::SelectionManager sel;
    int changeCount = 0;
    sel.SetOnChanged([&changeCount]() { changeCount++; });
    sel.ClearSelection();
    TEST_ASSERT_EQUAL_INT(0, changeCount);
}

void TestSelection::RunAllTests() {
    RUN_TEST(TestSelection::TestInitiallyEmpty);
    RUN_TEST(TestSelection::TestSelectSingle);
    RUN_TEST(TestSelection::TestSelectReplaces);
    RUN_TEST(TestSelection::TestMultiSelect);
    RUN_TEST(TestSelection::TestAddDuplicateNoop);
    RUN_TEST(TestSelection::TestRemoveFromSelection);
    RUN_TEST(TestSelection::TestToggleSelection);
    RUN_TEST(TestSelection::TestClearSelection);
    RUN_TEST(TestSelection::TestPrimary);
    RUN_TEST(TestSelection::TestOnChangedCallback);
    RUN_TEST(TestSelection::TestClearEmptyNoCallback);
}
