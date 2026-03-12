// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testeditorapp.cpp
 * @brief Tests for the editor application shell.
 * @date 03/09/2026
 * @author Coela
 */

#include "testeditorapp.hpp"

static koilo::EditorApp* s_app = nullptr;

static void SetupApp() {
    if (!s_app) {
        s_app = new koilo::EditorApp();
        s_app->Setup(1920, 1080);
    }
}

void TestEditorApp::TestSetupCreatesRoot() {
    SetupApp();
    int root = s_app->GetUI().GetRoot();
    TEST_ASSERT_TRUE(root >= 0);
}

void TestEditorApp::TestMenuBarCreated() {
    SetupApp();
    TEST_ASSERT_TRUE(s_app->GetMenuBar() >= 0);
    int found = s_app->GetUI().FindWidget("menuBar");
    TEST_ASSERT_EQUAL_INT(s_app->GetMenuBar(), found);
}

void TestEditorApp::TestStatusBarCreated() {
    SetupApp();
    TEST_ASSERT_TRUE(s_app->GetStatusBar() >= 0);
    int found = s_app->GetUI().FindWidget("statusBar");
    TEST_ASSERT_EQUAL_INT(s_app->GetStatusBar(), found);
}

void TestEditorApp::TestDockAreaCreated() {
    SetupApp();
    TEST_ASSERT_TRUE(s_app->GetDockArea() >= 0);
    int found = s_app->GetUI().FindWidget("dockArea");
    TEST_ASSERT_EQUAL_INT(s_app->GetDockArea(), found);
}

void TestEditorApp::TestPanesCreated() {
    SetupApp();
    TEST_ASSERT_TRUE(s_app->GetLeftPane() >= 0);
    TEST_ASSERT_TRUE(s_app->GetCenterPane() >= 0);
    TEST_ASSERT_TRUE(s_app->GetRightPane() >= 0);
}

void TestEditorApp::TestHierarchyWidgets() {
    SetupApp();
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("hierTitle") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("hierSearch") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("hierTree") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("hierCtxMenu") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("hierCtxAdd") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("hierCtxDel") >= 0);
}

void TestEditorApp::TestInspectorWidgets() {
    SetupApp();
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("inspTitle") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("inspScroll") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("inspNoSel") >= 0);
}

void TestEditorApp::TestViewportWidgets() {
    SetupApp();
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("vpToolbar") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("vpView") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("vpSelect") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("vpMove") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("vpRotate") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("vpScale") >= 0);
}

void TestEditorApp::TestConsoleWidgets() {
    SetupApp();
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("consTitle") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("consScroll") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("consLog") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("consInput") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("consClear") >= 0);
    TEST_ASSERT_TRUE(s_app->GetUI().FindWidget("consInfo") >= 0);
}

void TestEditorApp::TestWidgetCount() {
    SetupApp();
    int count = s_app->GetUI().GetWidgetCount();
    // Should have a substantial number of widgets from the full editor layout
    TEST_ASSERT_TRUE(count >= 40);
}

void TestEditorApp::TestUpdateFps() {
    SetupApp();
    // Run 60 frames at 60fps (1 second worth)
    for (int i = 0; i < 60; ++i) {
        s_app->Update(1.0f / 60.0f);
    }
    // FPS label should have been updated
    const char* fpsText = s_app->GetUI().GetText(
        s_app->GetUI().FindWidget("statusFps"));
    TEST_ASSERT_NOT_NULL(fpsText);
    // Should contain "FPS:" prefix after 1 second
    TEST_ASSERT_NOT_NULL(strstr(fpsText, "FPS:"));
}

void TestEditorApp::TestSelectionInitial() {
    SetupApp();
    TEST_ASSERT_TRUE(s_app->GetSelection().IsEmpty());
}

void TestEditorApp::TestUndoStackInitial() {
    SetupApp();
    TEST_ASSERT_FALSE(s_app->GetUndoStack().CanUndo());
    TEST_ASSERT_FALSE(s_app->GetUndoStack().CanRedo());
}

void TestEditorApp::TestLogBufferInitial() {
    SetupApp();
    // Setup pushes 2 log entries
    TEST_ASSERT_TRUE(s_app->GetLog().Count() >= 2);
}

void TestEditorApp::RunAllTests() {
    RUN_TEST(TestEditorApp::TestSetupCreatesRoot);
    RUN_TEST(TestEditorApp::TestMenuBarCreated);
    RUN_TEST(TestEditorApp::TestStatusBarCreated);
    RUN_TEST(TestEditorApp::TestDockAreaCreated);
    RUN_TEST(TestEditorApp::TestPanesCreated);
    RUN_TEST(TestEditorApp::TestHierarchyWidgets);
    RUN_TEST(TestEditorApp::TestInspectorWidgets);
    RUN_TEST(TestEditorApp::TestViewportWidgets);
    RUN_TEST(TestEditorApp::TestConsoleWidgets);
    RUN_TEST(TestEditorApp::TestWidgetCount);
    RUN_TEST(TestEditorApp::TestUpdateFps);
    RUN_TEST(TestEditorApp::TestSelectionInitial);
    RUN_TEST(TestEditorApp::TestUndoStackInitial);
    RUN_TEST(TestEditorApp::TestLogBufferInitial);

    // Cleanup
    delete s_app;
    s_app = nullptr;
}
