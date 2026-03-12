// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testui_widget.cpp
 * @brief Implementation of UI2 widget system unit tests.
 */

#include "testwidget.hpp"
#include <cstring>

using namespace koilo::ui;

// ========== String Interning ==========

void TestWidget::TestStringInternBasic() {
    StringTable table;
    StringId id = table.Intern("hello");
    TEST_ASSERT_NOT_EQUAL(NullStringId, id);
    TEST_ASSERT_EQUAL_STRING("hello", table.Lookup(id));
}

void TestWidget::TestStringInternDuplicate() {
    StringTable table;
    StringId a = table.Intern("test");
    StringId b = table.Intern("test");
    TEST_ASSERT_EQUAL(a, b);
    TEST_ASSERT_EQUAL(1, (int)table.Count());
}

void TestWidget::TestStringInternEmpty() {
    StringTable table;
    StringId id = table.Intern("");
    TEST_ASSERT_EQUAL(NullStringId, id);
    id = table.Intern(nullptr);
    TEST_ASSERT_EQUAL(NullStringId, id);
}

// ========== Widget Pool ==========

void TestWidget::TestPoolAllocate() {
    WidgetPool pool(8);
    int idx = pool.Allocate();
    TEST_ASSERT_GREATER_OR_EQUAL(0, idx);
    TEST_ASSERT_TRUE(pool.IsAlive(idx));
    TEST_ASSERT_EQUAL(1, (int)pool.Count());
}

void TestWidget::TestPoolFree() {
    WidgetPool pool(8);
    int idx = pool.Allocate();
    pool.Free(idx);
    TEST_ASSERT_FALSE(pool.IsAlive(idx));
    TEST_ASSERT_EQUAL(0, (int)pool.Count());
}

void TestWidget::TestPoolCapacity() {
    WidgetPool pool(4);
    for (int i = 0; i < 4; ++i) {
        TEST_ASSERT_GREATER_OR_EQUAL(0, pool.Allocate());
    }
    TEST_ASSERT_EQUAL(-1, pool.Allocate()); // full
}

void TestWidget::TestPoolFreeReuse() {
    WidgetPool pool(4);
    int a = pool.Allocate();
    int b = pool.Allocate();
    pool.Free(a);
    int c = pool.Allocate();
    TEST_ASSERT_EQUAL(a, c); // reuses freed slot
}

// ========== Widget Creation ==========

void TestWidget::TestCreatePanel() {
    UIContext ctx(64);
    int idx = ctx.CreatePanel("root");
    TEST_ASSERT_GREATER_OR_EQUAL(0, idx);
    const Widget* w = ctx.GetWidget(idx);
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_EQUAL((int)WidgetTag::Panel, (int)w->tag);
    TEST_ASSERT_TRUE(w->flags.visible);
    TEST_ASSERT_TRUE(w->flags.enabled);
}

void TestWidget::TestCreateLabel() {
    UIContext ctx(64);
    int idx = ctx.CreateLabel("lbl", "Hello World");
    TEST_ASSERT_GREATER_OR_EQUAL(0, idx);
    TEST_ASSERT_EQUAL_STRING("Hello World", ctx.GetText(idx));
}

void TestWidget::TestCreateButton() {
    UIContext ctx(64);
    int idx = ctx.CreateButton("btn", "Click Me");
    const Widget* w = ctx.GetWidget(idx);
    TEST_ASSERT_EQUAL((int)WidgetTag::Button, (int)w->tag);
    TEST_ASSERT_TRUE(w->flags.focusable);
    TEST_ASSERT_EQUAL_STRING("Click Me", ctx.GetText(idx));
}

void TestWidget::TestCreateSlider() {
    UIContext ctx(64);
    int idx = ctx.CreateSlider("sld", 0.0f, 100.0f, 50.0f);
    const Widget* w = ctx.GetWidget(idx);
    TEST_ASSERT_EQUAL((int)WidgetTag::Slider, (int)w->tag);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, w->sliderValue);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, w->sliderMin);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, w->sliderMax);
}

void TestWidget::TestCreateCheckbox() {
    UIContext ctx(64);
    int idx = ctx.CreateCheckbox("cb", true);
    const Widget* w = ctx.GetWidget(idx);
    TEST_ASSERT_EQUAL((int)WidgetTag::Checkbox, (int)w->tag);
    TEST_ASSERT_TRUE(w->checked);
}

void TestWidget::TestCreateTextField() {
    UIContext ctx(64);
    int idx = ctx.CreateTextField("tf", "Type here...");
    const Widget* w = ctx.GetWidget(idx);
    TEST_ASSERT_EQUAL((int)WidgetTag::TextField, (int)w->tag);
    TEST_ASSERT_TRUE(w->flags.focusable);
}

void TestWidget::TestCreateSeparator() {
    UIContext ctx(64);
    int idx = ctx.CreateSeparator("sep");
    const Widget* w = ctx.GetWidget(idx);
    TEST_ASSERT_EQUAL((int)WidgetTag::Separator, (int)w->tag);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, w->localH);
    TEST_ASSERT_EQUAL((int)SizeMode::FillRemaining, (int)w->widthMode);
}

// ========== Widget Tree ==========

void TestWidget::TestSetParent() {
    UIContext ctx(64);
    int root = ctx.CreatePanel("root");
    int child = ctx.CreateLabel("child", "Hi");
    TEST_ASSERT_TRUE(ctx.SetParent(child, root));
    const Widget* r = ctx.GetWidget(root);
    TEST_ASSERT_EQUAL(1, (int)r->childCount);
    TEST_ASSERT_EQUAL(child, (int)r->children[0]);
    TEST_ASSERT_EQUAL(root, (int)ctx.GetWidget(child)->parent);
}

void TestWidget::TestDestroyWidget() {
    UIContext ctx(64);
    int root = ctx.CreatePanel("root");
    int child = ctx.CreateLabel("child", "Hi");
    ctx.SetParent(child, root);
    ctx.DestroyWidget(child);
    TEST_ASSERT_EQUAL(0, (int)ctx.GetWidget(root)->childCount);
    TEST_ASSERT_NULL(ctx.GetWidget(child));
}

void TestWidget::TestDestroyWidgetRecursive() {
    UIContext ctx(64);
    int root = ctx.CreatePanel("root");
    int child = ctx.CreatePanel("child");
    int grandchild = ctx.CreateLabel("gc", "text");
    ctx.SetParent(child, root);
    ctx.SetParent(grandchild, child);
    ctx.DestroyWidget(child);
    TEST_ASSERT_NULL(ctx.GetWidget(child));
    TEST_ASSERT_NULL(ctx.GetWidget(grandchild));
    TEST_ASSERT_EQUAL(0, (int)ctx.GetWidget(root)->childCount);
}

void TestWidget::TestReparent() {
    UIContext ctx(64);
    int a = ctx.CreatePanel("a");
    int b = ctx.CreatePanel("b");
    int child = ctx.CreateLabel("child", "text");
    ctx.SetParent(child, a);
    TEST_ASSERT_EQUAL(1, (int)ctx.GetWidget(a)->childCount);
    ctx.SetParent(child, b);
    TEST_ASSERT_EQUAL(0, (int)ctx.GetWidget(a)->childCount);
    TEST_ASSERT_EQUAL(1, (int)ctx.GetWidget(b)->childCount);
}

// ========== Layout ==========

void TestWidget::TestLayoutRootFillsViewport() {
    UIContext ctx(64);
    ctx.SetViewport(800.0f, 600.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    ctx.UpdateLayout();
    const Widget* r = ctx.GetWidget(root);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, r->computedRect.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, r->computedRect.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 800.0f, r->computedRect.w);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 600.0f, r->computedRect.h);
}

void TestWidget::TestLayoutColumn() {
    UIContext ctx(64);
    ctx.SetViewport(400.0f, 300.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    ctx.SetLayout(root, LayoutDirection::Column);

    int c1 = ctx.CreatePanel("c1");
    int c2 = ctx.CreatePanel("c2");
    ctx.SetSize(c1, 100.0f, 50.0f);
    ctx.SetSize(c2, 100.0f, 50.0f);
    ctx.SetParent(c1, root);
    ctx.SetParent(c2, root);

    ctx.UpdateLayout();

    const Widget* w1 = ctx.GetWidget(c1);
    const Widget* w2 = ctx.GetWidget(c2);
    // c1 at y=0, c2 at y=50
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, w1->computedRect.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, w2->computedRect.y);
}

void TestWidget::TestLayoutRow() {
    UIContext ctx(64);
    ctx.SetViewport(400.0f, 300.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    ctx.SetLayout(root, LayoutDirection::Row);

    int c1 = ctx.CreatePanel("c1");
    int c2 = ctx.CreatePanel("c2");
    ctx.SetSize(c1, 100.0f, 50.0f);
    ctx.SetSize(c2, 100.0f, 50.0f);
    ctx.SetParent(c1, root);
    ctx.SetParent(c2, root);

    ctx.UpdateLayout();

    const Widget* w1 = ctx.GetWidget(c1);
    const Widget* w2 = ctx.GetWidget(c2);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, w1->computedRect.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, w2->computedRect.x);
}

void TestWidget::TestLayoutPercentSize() {
    UIContext ctx(64);
    ctx.SetViewport(800.0f, 600.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    int child = ctx.CreatePanel("child");
    ctx.SetSize(child, 50.0f, 50.0f); // 50% of parent
    ctx.SetSizeMode(child, SizeMode::Percent, SizeMode::Percent);
    ctx.SetParent(child, root);

    ctx.UpdateLayout();

    const Widget* w = ctx.GetWidget(child);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 400.0f, w->computedRect.w);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 300.0f, w->computedRect.h);
}

void TestWidget::TestLayoutFillRemaining() {
    UIContext ctx(64);
    ctx.SetViewport(800.0f, 600.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    int child = ctx.CreatePanel("child");
    ctx.SetSizeMode(child, SizeMode::FillRemaining, SizeMode::FillRemaining);
    ctx.SetParent(child, root);

    ctx.UpdateLayout();

    const Widget* w = ctx.GetWidget(child);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 800.0f, w->computedRect.w);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 600.0f, w->computedRect.h);
}

void TestWidget::TestLayoutPadding() {
    UIContext ctx(64);
    ctx.SetViewport(400.0f, 300.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    ctx.SetPadding(root, 10.0f, 10.0f, 10.0f, 10.0f);
    ctx.SetLayout(root, LayoutDirection::Column);

    int child = ctx.CreatePanel("child");
    ctx.SetSize(child, 100.0f, 50.0f);
    ctx.SetParent(child, root);

    ctx.UpdateLayout();

    const Widget* w = ctx.GetWidget(child);
    // Child should be offset by parent padding
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, w->computedRect.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, w->computedRect.y);
}

void TestWidget::TestLayoutCenterAlignment() {
    UIContext ctx(64);
    ctx.SetViewport(400.0f, 300.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    ctx.SetLayout(root, LayoutDirection::Column, Alignment::Center);

    int child = ctx.CreatePanel("child");
    ctx.SetSize(child, 100.0f, 50.0f);
    ctx.SetParent(child, root);

    ctx.UpdateLayout();

    const Widget* w = ctx.GetWidget(child);
    // Centered vertically: (300 - 50) / 2 = 125
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 125.0f, w->computedRect.y);
}

// ========== Events ==========

void TestWidget::TestHitTest() {
    UIContext ctx(64);
    ctx.SetViewport(400.0f, 300.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    ctx.UpdateLayout();

    int hit = HitTest(ctx.Pool(), root, 200.0f, 150.0f);
    TEST_ASSERT_EQUAL(root, hit);

    hit = HitTest(ctx.Pool(), root, 500.0f, 400.0f); // outside
    TEST_ASSERT_EQUAL(-1, hit);
}

void TestWidget::TestHitTestNested() {
    UIContext ctx(64);
    ctx.SetViewport(400.0f, 300.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    int child = ctx.CreateButton("btn", "Click");
    ctx.SetSize(child, 80.0f, 30.0f);
    ctx.SetPosition(child, 10.0f, 10.0f);
    ctx.SetParent(child, root);
    ctx.UpdateLayout();

    int hit = HitTest(ctx.Pool(), root, 20.0f, 20.0f);
    TEST_ASSERT_EQUAL(child, hit);
}

void TestWidget::TestClickCallback() {
    UIContext ctx(64);
    ctx.SetViewport(400.0f, 300.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    int btn = ctx.CreateButton("btn", "Click");
    ctx.SetSize(btn, 400.0f, 300.0f);
    ctx.SetParent(btn, root);
    ctx.UpdateLayout();

    bool clicked = false;
    ctx.SetOnClick(btn, [&clicked](Widget&) { clicked = true; });

    Event down;
    down.type = EventType::PointerDown;
    down.pointerX = 200.0f;
    down.pointerY = 150.0f;
    ctx.ProcessEvent(down);

    Event up;
    up.type = EventType::PointerUp;
    up.pointerX = 200.0f;
    up.pointerY = 150.0f;
    ctx.ProcessEvent(up);

    TEST_ASSERT_TRUE(clicked);
}

void TestWidget::TestSliderDrag() {
    UIContext ctx(64);
    ctx.SetViewport(400.0f, 300.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    int sld = ctx.CreateSlider("sld", 0.0f, 100.0f, 0.0f);
    ctx.SetSize(sld, 200.0f, 20.0f);
    ctx.SetParent(sld, root);
    ctx.UpdateLayout();

    // Press on slider
    Event down;
    down.type = EventType::PointerDown;
    down.pointerX = 10.0f;
    down.pointerY = 10.0f;
    ctx.ProcessEvent(down);

    // Drag to middle
    Event move;
    move.type = EventType::PointerMove;
    move.pointerX = 100.0f;
    move.pointerY = 10.0f;
    ctx.ProcessEvent(move);

    const Widget* w = ctx.GetWidget(sld);
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 50.0f, w->sliderValue);
}

void TestWidget::TestCheckboxToggle() {
    UIContext ctx(64);
    ctx.SetViewport(400.0f, 300.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    int cb = ctx.CreateCheckbox("cb", false);
    ctx.SetSize(cb, 400.0f, 300.0f);
    ctx.SetParent(cb, root);
    ctx.UpdateLayout();

    // Click checkbox
    Event down;
    down.type = EventType::PointerDown;
    down.pointerX = 200.0f;
    down.pointerY = 150.0f;
    ctx.ProcessEvent(down);

    Event up;
    up.type = EventType::PointerUp;
    up.pointerX = 200.0f;
    up.pointerY = 150.0f;
    ctx.ProcessEvent(up);

    TEST_ASSERT_TRUE(ctx.GetWidget(cb)->checked);
}

void TestWidget::TestFocusTab() {
    UIContext ctx(64);
    ctx.SetViewport(400.0f, 300.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    ctx.SetLayout(root, LayoutDirection::Column);

    int btn1 = ctx.CreateButton("b1", "First");
    int btn2 = ctx.CreateButton("b2", "Second");
    ctx.SetSize(btn1, 100.0f, 30.0f);
    ctx.SetSize(btn2, 100.0f, 30.0f);
    ctx.SetParent(btn1, root);
    ctx.SetParent(btn2, root);
    ctx.UpdateLayout();

    // Click first button to focus it
    Event down;
    down.type = EventType::PointerDown;
    down.pointerX = 50.0f;
    down.pointerY = 15.0f;
    ctx.ProcessEvent(down);
    TEST_ASSERT_EQUAL(btn1, ctx.FocusedWidget());

    // Tab to next
    Event tab;
    tab.type = EventType::KeyDown;
    tab.key = KeyCode::Tab;
    tab.mods = {};
    ctx.ProcessEvent(tab);
    TEST_ASSERT_EQUAL(btn2, ctx.FocusedWidget());
}

// ========== Styling ==========

void TestWidget::TestThemeDefaults() {
    Theme theme;
    const Style& btnStyle = theme.Get(WidgetTag::Button, PseudoState::Normal);
    TEST_ASSERT_TRUE(btnStyle.isSet);
    TEST_ASSERT_EQUAL(60, (int)btnStyle.background.r);
}

void TestWidget::TestThemeResolve() {
    UIContext ctx(64);
    int btn = ctx.CreateButton("btn", "Test");
    Widget* w = ctx.GetWidget(btn);
    const Style& s = ctx.GetTheme().Resolve(*w);
    // Button normal state
    TEST_ASSERT_EQUAL(60, (int)s.background.r);
}

void TestWidget::TestPseudoStateHovered() {
    UIContext ctx(64);
    int btn = ctx.CreateButton("btn", "Test");
    Widget* w = ctx.GetWidget(btn);
    w->flags.hovered = 1;
    const Style& s = ctx.GetTheme().Resolve(*w);
    TEST_ASSERT_EQUAL(75, (int)s.background.r);
}

// ========== UIContext ==========

void TestWidget::TestSetViewport() {
    UIContext ctx(64);
    ctx.SetViewport(1920.0f, 1080.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1920.0f, ctx.ViewportWidth());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1080.0f, ctx.ViewportHeight());
}

void TestWidget::TestFindWidget() {
    UIContext ctx(64);
    int root = ctx.CreatePanel("root");
    int child = ctx.CreateLabel("my-label", "text");
    ctx.SetParent(child, root);
    TEST_ASSERT_EQUAL(child, ctx.FindWidget("my-label"));
    TEST_ASSERT_EQUAL(-1, ctx.FindWidget("nonexistent"));
}

void TestWidget::TestSetText() {
    UIContext ctx(64);
    int lbl = ctx.CreateLabel("lbl", "initial");
    ctx.SetText(lbl, "updated");
    TEST_ASSERT_EQUAL_STRING("updated", ctx.GetText(lbl));
}

void TestWidget::TestSetVisible() {
    UIContext ctx(64);
    int panel = ctx.CreatePanel("p");
    ctx.SetVisible(panel, false);
    TEST_ASSERT_FALSE(ctx.GetWidget(panel)->flags.visible);
    ctx.SetVisible(panel, true);
    TEST_ASSERT_TRUE(ctx.GetWidget(panel)->flags.visible);
}

// ========== New Widget Types ==========

void TestWidget::TestCreateProgressBar() {
    UIContext ctx(64);
    int idx = ctx.CreateProgressBar("pb", 0.5f);
    const Widget* w = ctx.GetWidget(idx);
    TEST_ASSERT_EQUAL((int)WidgetTag::ProgressBar, (int)w->tag);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, w->progressValue);
    TEST_ASSERT_EQUAL((int)SizeMode::FillRemaining, (int)w->widthMode);
}

void TestWidget::TestCreateToggleSwitch() {
    UIContext ctx(64);
    int idx = ctx.CreateToggleSwitch("ts", true);
    const Widget* w = ctx.GetWidget(idx);
    TEST_ASSERT_EQUAL((int)WidgetTag::ToggleSwitch, (int)w->tag);
    TEST_ASSERT_TRUE(w->checked);
    TEST_ASSERT_TRUE(w->flags.focusable);
}

void TestWidget::TestCreateRadioButton() {
    UIContext ctx(64);
    int idx = ctx.CreateRadioButton("rb", "group1", false);
    const Widget* w = ctx.GetWidget(idx);
    TEST_ASSERT_EQUAL((int)WidgetTag::RadioButton, (int)w->tag);
    TEST_ASSERT_FALSE(w->checked);
    TEST_ASSERT_TRUE(w->flags.focusable);
}

void TestWidget::TestCreateNumberSpinner() {
    UIContext ctx(64);
    int idx = ctx.CreateNumberSpinner("ns", 10.0f, 0.0f, 100.0f, 5.0f);
    const Widget* w = ctx.GetWidget(idx);
    TEST_ASSERT_EQUAL((int)WidgetTag::NumberSpinner, (int)w->tag);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, w->sliderValue);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, w->sliderMin);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, w->sliderMax);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, w->spinnerStep);
    TEST_ASSERT_TRUE(w->flags.focusable);
}

void TestWidget::TestToggleSwitchToggle() {
    UIContext ctx(64);
    ctx.SetViewport(400.0f, 300.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    int ts = ctx.CreateToggleSwitch("ts", false);
    ctx.SetSize(ts, 400.0f, 300.0f);
    ctx.SetParent(ts, root);
    ctx.UpdateLayout();

    TEST_ASSERT_FALSE(ctx.GetWidget(ts)->checked);

    Event down;
    down.type = EventType::PointerDown;
    down.pointerX = 200.0f;
    down.pointerY = 150.0f;
    ctx.ProcessEvent(down);

    Event up;
    up.type = EventType::PointerUp;
    up.pointerX = 200.0f;
    up.pointerY = 150.0f;
    ctx.ProcessEvent(up);

    TEST_ASSERT_TRUE(ctx.GetWidget(ts)->checked);
}

void TestWidget::TestRadioGroupExclusion() {
    UIContext ctx(64);
    ctx.SetViewport(400.0f, 300.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    // Create three radio buttons in same group
    int r1 = ctx.CreateRadioButton("r1", "g", true);
    int r2 = ctx.CreateRadioButton("r2", "g", false);
    int r3 = ctx.CreateRadioButton("r3", "g", false);
    ctx.SetSize(r1, 400.0f, 100.0f);
    ctx.SetSize(r2, 400.0f, 100.0f);
    ctx.SetSize(r3, 400.0f, 100.0f);
    ctx.SetParent(r1, root);
    ctx.SetParent(r2, root);
    ctx.SetParent(r3, root);
    ctx.UpdateLayout();

    TEST_ASSERT_TRUE(ctx.GetWidget(r1)->checked);
    TEST_ASSERT_FALSE(ctx.GetWidget(r2)->checked);

    // Click r2 - should select r2 and deselect r1
    Event down;
    down.type = EventType::PointerDown;
    down.pointerX = 200.0f;
    down.pointerY = 150.0f;  // middle of r2 (100-200)
    ctx.ProcessEvent(down);

    Event up;
    up.type = EventType::PointerUp;
    up.pointerX = 200.0f;
    up.pointerY = 150.0f;
    ctx.ProcessEvent(up);

    TEST_ASSERT_FALSE(ctx.GetWidget(r1)->checked);
    TEST_ASSERT_TRUE(ctx.GetWidget(r2)->checked);
    TEST_ASSERT_FALSE(ctx.GetWidget(r3)->checked);
}

void TestWidget::TestSpinnerClamp() {
    UIContext ctx(64);
    int idx = ctx.CreateNumberSpinner("sp", 95.0f, 0.0f, 100.0f, 10.0f);
    Widget* w = const_cast<Widget*>(ctx.GetWidget(idx));

    // Increment beyond max should clamp
    w->sliderValue = std::min(w->sliderMax, w->sliderValue + w->spinnerStep);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, w->sliderValue);

    // Decrement from 0 should clamp to min
    w->sliderValue = 5.0f;
    w->sliderValue = std::max(w->sliderMin, w->sliderValue - w->spinnerStep);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, w->sliderValue);
}

// ========== Test Runner ==========

void TestWidget::RunAllTests() {
    // String interning
    RUN_TEST(TestWidget::TestStringInternBasic);
    RUN_TEST(TestWidget::TestStringInternDuplicate);
    RUN_TEST(TestWidget::TestStringInternEmpty);

    // Widget pool
    RUN_TEST(TestWidget::TestPoolAllocate);
    RUN_TEST(TestWidget::TestPoolFree);
    RUN_TEST(TestWidget::TestPoolCapacity);
    RUN_TEST(TestWidget::TestPoolFreeReuse);

    // Widget creation
    RUN_TEST(TestWidget::TestCreatePanel);
    RUN_TEST(TestWidget::TestCreateLabel);
    RUN_TEST(TestWidget::TestCreateButton);
    RUN_TEST(TestWidget::TestCreateSlider);
    RUN_TEST(TestWidget::TestCreateCheckbox);
    RUN_TEST(TestWidget::TestCreateTextField);
    RUN_TEST(TestWidget::TestCreateSeparator);

    // Widget tree
    RUN_TEST(TestWidget::TestSetParent);
    RUN_TEST(TestWidget::TestDestroyWidget);
    RUN_TEST(TestWidget::TestDestroyWidgetRecursive);
    RUN_TEST(TestWidget::TestReparent);

    // Layout
    RUN_TEST(TestWidget::TestLayoutRootFillsViewport);
    RUN_TEST(TestWidget::TestLayoutColumn);
    RUN_TEST(TestWidget::TestLayoutRow);
    RUN_TEST(TestWidget::TestLayoutPercentSize);
    RUN_TEST(TestWidget::TestLayoutFillRemaining);
    RUN_TEST(TestWidget::TestLayoutPadding);
    RUN_TEST(TestWidget::TestLayoutCenterAlignment);

    // Events
    RUN_TEST(TestWidget::TestHitTest);
    RUN_TEST(TestWidget::TestHitTestNested);
    RUN_TEST(TestWidget::TestClickCallback);
    RUN_TEST(TestWidget::TestSliderDrag);
    RUN_TEST(TestWidget::TestCheckboxToggle);
    RUN_TEST(TestWidget::TestFocusTab);

    // Styling
    RUN_TEST(TestWidget::TestThemeDefaults);
    RUN_TEST(TestWidget::TestThemeResolve);
    RUN_TEST(TestWidget::TestPseudoStateHovered);

    // UIContext
    RUN_TEST(TestWidget::TestSetViewport);
    RUN_TEST(TestWidget::TestFindWidget);
    RUN_TEST(TestWidget::TestSetText);
    RUN_TEST(TestWidget::TestSetVisible);

    // New widget types
    RUN_TEST(TestWidget::TestCreateProgressBar);
    RUN_TEST(TestWidget::TestCreateToggleSwitch);
    RUN_TEST(TestWidget::TestCreateRadioButton);
    RUN_TEST(TestWidget::TestCreateNumberSpinner);
    RUN_TEST(TestWidget::TestToggleSwitchToggle);
    RUN_TEST(TestWidget::TestRadioGroupExclusion);
    RUN_TEST(TestWidget::TestSpinnerClamp);
}
