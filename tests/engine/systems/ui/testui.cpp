// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testui.cpp
 * @brief Implementation of UI wrapper unit tests.
 */

#include "testui.hpp"

using namespace koilo;

void TestUI::TestDefaultConstructor() {
    UI ui;
    // Root widget should exist
    TEST_ASSERT_GREATER_OR_EQUAL(0, ui.Context().Root());
}

void TestUI::TestClear() {
    UI ui;
    int btn = ui.Context().CreateButton("btn", "Test");
    ui.Context().SetParent(btn, ui.Context().Root());
    ui.Clear();
    // After clear, root should still exist but no user widgets
    TEST_ASSERT_GREATER_OR_EQUAL(0, ui.Context().Root());
}

void TestUI::TestRenderToBuffer() {
    UI ui;
    // RenderToBuffer should not crash with nullptr
    ui.RenderToBuffer(nullptr, 800, 600);
    // Should update viewport
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 800.0f, ui.Context().ViewportWidth());
}

void TestUI::TestContextAccess() {
    UI ui;
    int lbl = ui.Context().CreateLabel("lbl", "Hello");
    TEST_ASSERT_GREATER_OR_EQUAL(0, lbl);
    TEST_ASSERT_EQUAL_STRING("Hello", ui.Context().GetText(lbl));
}

void TestUI::RunAllTests() {
    RUN_TEST(TestUI::TestDefaultConstructor);
    RUN_TEST(TestUI::TestClear);
    RUN_TEST(TestUI::TestRenderToBuffer);
    RUN_TEST(TestUI::TestContextAccess);
}
