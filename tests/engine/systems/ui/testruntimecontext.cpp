// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testruntimecontext.cpp
 * @brief Unit tests for RuntimeContext isolation.
 */

#include "testruntimecontext.hpp"
#include <koilo/systems/ui/runtime_context.hpp>

using namespace koilo::ui;

void TestRuntimeContext::TestConstructEditor() {
    RuntimeContext ctx(ContextRole::Editor, 50 * 1024 * 1024);
    TEST_ASSERT_EQUAL(ContextRole::Editor, ctx.Role());
    TEST_ASSERT_EQUAL_STRING("Editor", ctx.Tag());
    TEST_ASSERT_EQUAL(50u * 1024 * 1024, ctx.MemoryBudget());
    TEST_ASSERT_EQUAL(0u, ctx.MemoryUsage());
}

void TestRuntimeContext::TestConstructGame() {
    RuntimeContext ctx(ContextRole::Game, 20 * 1024 * 1024);
    TEST_ASSERT_EQUAL(ContextRole::Game, ctx.Role());
    TEST_ASSERT_EQUAL_STRING("Game", ctx.Tag());
    TEST_ASSERT_EQUAL(20u * 1024 * 1024, ctx.MemoryBudget());
}

void TestRuntimeContext::TestMemoryBudgetAlloc() {
    RuntimeContext ctx(ContextRole::Game, 1024);
    TEST_ASSERT_TRUE(ctx.TryAllocate(512));
    TEST_ASSERT_EQUAL(512u, ctx.MemoryUsage());
    TEST_ASSERT_TRUE(ctx.TryAllocate(512));
    TEST_ASSERT_EQUAL(1024u, ctx.MemoryUsage());
}

void TestRuntimeContext::TestMemoryBudgetExceed() {
    RuntimeContext ctx(ContextRole::Game, 1024);
    TEST_ASSERT_TRUE(ctx.TryAllocate(800));
    TEST_ASSERT_FALSE(ctx.TryAllocate(300));
    // Usage should not change on failed allocation
    TEST_ASSERT_EQUAL(800u, ctx.MemoryUsage());
}

void TestRuntimeContext::TestMemoryRelease() {
    RuntimeContext ctx(ContextRole::Game, 1024);
    ctx.TryAllocate(600);
    ctx.Release(400);
    TEST_ASSERT_EQUAL(200u, ctx.MemoryUsage());
    // Release more than usage clamps to 0
    ctx.Release(9999);
    TEST_ASSERT_EQUAL(0u, ctx.MemoryUsage());
}

void TestRuntimeContext::TestPeakMemory() {
    RuntimeContext ctx(ContextRole::Game, 4096);
    ctx.TryAllocate(1000);
    ctx.TryAllocate(500);
    ctx.Release(800);
    ctx.TryAllocate(200);
    TEST_ASSERT_EQUAL(1500u, ctx.PeakMemoryUsage());
}

void TestRuntimeContext::TestErrorState() {
    RuntimeContext ctx(ContextRole::Game);
    TEST_ASSERT_FALSE(ctx.HasError());
    ctx.SetError("script crashed");
    TEST_ASSERT_TRUE(ctx.HasError());
    TEST_ASSERT_EQUAL_STRING("script crashed", ctx.LastError());
}

void TestRuntimeContext::TestClearError() {
    RuntimeContext ctx(ContextRole::Game);
    ctx.SetError("oops");
    ctx.ClearError();
    TEST_ASSERT_FALSE(ctx.HasError());
    TEST_ASSERT_EQUAL_STRING("", ctx.LastError());
}

void TestRuntimeContext::TestSnapshotRestore() {
    RuntimeContext ctx(ContextRole::Editor);
    UIContext& ui = ctx.UI();

    int root = ui.CreatePanel("root");
    ui.SetRoot(root);
    int btn = ui.CreateButton("my_btn", "Click");
    ui.SetParent(btn, root);

    ctx.SnapshotWidgetState();

    // Destroy and recreate (simulating reload)
    ui.DestroyWidget(btn);
    int btn2 = ui.CreateButton("my_btn", "Click");
    ui.SetParent(btn2, root);

    ctx.RestoreWidgetState();
    // Should succeed without crash - button still exists
    Widget* w = ui.GetWidget(btn2);
    TEST_ASSERT_NOT_NULL(w);
}

void TestRuntimeContext::TestSnapshotPreservesSlider() {
    RuntimeContext ctx(ContextRole::Editor);
    UIContext& ui = ctx.UI();

    int root = ui.CreatePanel("root");
    ui.SetRoot(root);
    int sld = ui.CreateSlider("vol_slider", 0.0f, 1.0f, 0.0f);
    ui.SetParent(sld, root);

    // User adjusts slider
    Widget* w = ui.GetWidget(sld);
    w->sliderValue = 0.75f;

    ctx.SnapshotWidgetState();

    // Simulate hot-reload: destroy and recreate
    ui.DestroyWidget(sld);
    int sld2 = ui.CreateSlider("vol_slider", 0.0f, 1.0f, 0.0f);
    ui.SetParent(sld2, root);

    ctx.RestoreWidgetState();

    Widget* w2 = ui.GetWidget(sld2);
    TEST_ASSERT_NOT_NULL(w2);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.75f, w2->sliderValue);
}

void TestRuntimeContext::TestSnapshotPreservesCheckbox() {
    RuntimeContext ctx(ContextRole::Game);
    UIContext& ui = ctx.UI();

    int root = ui.CreatePanel("root");
    ui.SetRoot(root);
    int chk = ui.CreateCheckbox("opt_chk", false);
    ui.SetParent(chk, root);

    Widget* w = ui.GetWidget(chk);
    w->checked = true;

    ctx.SnapshotWidgetState();

    ui.DestroyWidget(chk);
    int chk2 = ui.CreateCheckbox("opt_chk", false);
    ui.SetParent(chk2, root);

    ctx.RestoreWidgetState();

    Widget* w2 = ui.GetWidget(chk2);
    TEST_ASSERT_NOT_NULL(w2);
    TEST_ASSERT_TRUE(w2->checked);
}

void TestRuntimeContext::TestIsolatedContexts() {
    RuntimeContext editor(ContextRole::Editor, 50 * 1024 * 1024);
    RuntimeContext game(ContextRole::Game, 20 * 1024 * 1024);

    // Create widgets in editor
    int eRoot = editor.UI().CreatePanel("e_root");
    editor.UI().SetRoot(eRoot);
    editor.UI().CreateButton("e_btn", "Edit");

    // Create widgets in game
    int gRoot = game.UI().CreatePanel("g_root");
    game.UI().SetRoot(gRoot);
    game.UI().CreateButton("g_btn", "Play");

    // Game error should not affect editor
    game.SetError("game script crashed");
    TEST_ASSERT_TRUE(game.HasError());
    TEST_ASSERT_FALSE(editor.HasError());

    // Widget trees are independent
    TEST_ASSERT_EQUAL(-1, editor.UI().FindWidget("g_btn"));
    TEST_ASSERT_EQUAL(-1, game.UI().FindWidget("e_btn"));
    TEST_ASSERT(editor.UI().FindWidget("e_btn") >= 0);
    TEST_ASSERT(game.UI().FindWidget("g_btn") >= 0);
}

void TestRuntimeContext::RunAllTests() {
    RUN_TEST(TestRuntimeContext::TestConstructEditor);
    RUN_TEST(TestRuntimeContext::TestConstructGame);
    RUN_TEST(TestRuntimeContext::TestMemoryBudgetAlloc);
    RUN_TEST(TestRuntimeContext::TestMemoryBudgetExceed);
    RUN_TEST(TestRuntimeContext::TestMemoryRelease);
    RUN_TEST(TestRuntimeContext::TestPeakMemory);
    RUN_TEST(TestRuntimeContext::TestErrorState);
    RUN_TEST(TestRuntimeContext::TestClearError);
    RUN_TEST(TestRuntimeContext::TestSnapshotRestore);
    RUN_TEST(TestRuntimeContext::TestSnapshotPreservesSlider);
    RUN_TEST(TestRuntimeContext::TestSnapshotPreservesCheckbox);
    RUN_TEST(TestRuntimeContext::TestIsolatedContexts);
}
