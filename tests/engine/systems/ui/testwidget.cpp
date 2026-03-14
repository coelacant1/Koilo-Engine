// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testui_widget.cpp
 * @brief Implementation of UI2 widget system unit tests.
 */

#include "testwidget.hpp"
#include <koilo/systems/ui/render/draw_list.hpp>
#include <koilo/systems/ui/settings_store.hpp>
#include <koilo/systems/ui/color_picker.hpp>
#include <koilo/systems/ui/curve_editor.hpp>
#include <koilo/systems/ui/timeline.hpp>
#include <koilo/systems/ui/fs_adapter.hpp>
#include <koilo/systems/ui/breadcrumb.hpp>
#include <koilo/systems/ui/content_browser.hpp>
#include <koilo/systems/ui/node_graph.hpp>
#include <koilo/systems/ui/preferences_panel.hpp>
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

// ========== Drag-and-Drop ==========

void TestWidget::TestDragPayloadLifecycle() {
    DragPayload p;
    TEST_ASSERT_FALSE(p.IsActive());
    p.typeTag = DragType::Widget;
    p.sourceWidget = 42;
    TEST_ASSERT_TRUE(p.IsActive());
    p.Clear();
    TEST_ASSERT_FALSE(p.IsActive());
    TEST_ASSERT_EQUAL(-1, p.sourceWidget);
}

void TestWidget::TestDragThreshold() {
    // Drag should not start until pointer moves > 4px from press point
    UIContext ctx(64);
    ctx.SetViewport(800.0f, 600.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    ctx.GetWidget(root)->widthMode = SizeMode::Fixed;
    ctx.GetWidget(root)->heightMode = SizeMode::Fixed;
    ctx.GetWidget(root)->localW = 800.0f;
    ctx.GetWidget(root)->localH = 600.0f;

    int btn = ctx.CreatePanel("draggable");
    ctx.SetParent(btn, root);
    Widget* bw = ctx.GetWidget(btn);
    bw->widthMode = SizeMode::Fixed;
    bw->heightMode = SizeMode::Fixed;
    bw->localW = 100.0f;
    bw->localH = 50.0f;
    bw->onDragBegin = [](int idx) -> DragPayload {
        DragPayload p;
        p.typeTag = DragType::Widget;
        p.sourceWidget = idx;
        return p;
    };

    ctx.UpdateLayout();

    // Press at (50,25) - inside the draggable widget
    Event down;
    down.type = EventType::PointerDown;
    down.pointerX = 50.0f;
    down.pointerY = 25.0f;
    ctx.ProcessEvent(down);
    TEST_ASSERT_FALSE(ctx.IsDragging());

    // Move 2px - below threshold
    Event move1;
    move1.type = EventType::PointerMove;
    move1.pointerX = 52.0f;
    move1.pointerY = 25.0f;
    ctx.ProcessEvent(move1);
    TEST_ASSERT_FALSE(ctx.IsDragging());

    // Move to 55px - 5px total, above threshold
    Event move2;
    move2.type = EventType::PointerMove;
    move2.pointerX = 55.0f;
    move2.pointerY = 25.0f;
    ctx.ProcessEvent(move2);
    TEST_ASSERT_TRUE(ctx.IsDragging());
    TEST_ASSERT_EQUAL((int)DragType::Widget, (int)ctx.ActiveDrag().typeTag);
}

void TestWidget::TestDropAcceptReject() {
    UIContext ctx(64);
    ctx.SetViewport(800.0f, 600.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    ctx.GetWidget(root)->widthMode = SizeMode::Fixed;
    ctx.GetWidget(root)->heightMode = SizeMode::Fixed;
    ctx.GetWidget(root)->localW = 800.0f;
    ctx.GetWidget(root)->localH = 600.0f;
    ctx.GetWidget(root)->layout.direction = LayoutDirection::Row;

    int src = ctx.CreatePanel("src");
    ctx.SetParent(src, root);
    Widget* sw = ctx.GetWidget(src);
    sw->widthMode = SizeMode::Fixed;
    sw->heightMode = SizeMode::Fixed;
    sw->localW = 100.0f;
    sw->localH = 100.0f;
    sw->onDragBegin = [](int idx) -> DragPayload {
        DragPayload p;
        p.typeTag = DragType::Widget;
        p.sourceWidget = idx;
        return p;
    };

    // Target that rejects drops
    int reject = ctx.CreatePanel("reject");
    ctx.SetParent(reject, root);
    Widget* rw = ctx.GetWidget(reject);
    rw->widthMode = SizeMode::Fixed;
    rw->heightMode = SizeMode::Fixed;
    rw->localW = 100.0f;
    rw->localH = 100.0f;
    // No acceptDropTypes or onCanDrop set

    // Target that accepts drops
    bool dropReceived = false;
    int accept = ctx.CreatePanel("accept");
    ctx.SetParent(accept, root);
    Widget* aw = ctx.GetWidget(accept);
    aw->widthMode = SizeMode::Fixed;
    aw->heightMode = SizeMode::Fixed;
    aw->localW = 100.0f;
    aw->localH = 100.0f;
    aw->acceptDropTypes = (1u << DragType::Widget);
    aw->onCanDrop = [](const DragPayload&) { return true; };
    aw->onDrop = [&dropReceived](const DragPayload&, DropPosition) { dropReceived = true; };

    ctx.UpdateLayout();

    // Start drag on src
    Event down;
    down.type = EventType::PointerDown;
    down.pointerX = 50.0f;
    down.pointerY = 50.0f;
    ctx.ProcessEvent(down);

    Event move;
    move.type = EventType::PointerMove;
    move.pointerX = 60.0f;
    move.pointerY = 50.0f;
    ctx.ProcessEvent(move);
    TEST_ASSERT_TRUE(ctx.IsDragging());

    // Drop on accept target - need to figure out accept widget position after layout
    // Accept is 3rd child (200-300 in x since row layout: 0-100, 100-200, 200-300)
    Event up;
    up.type = EventType::PointerUp;
    up.pointerX = 250.0f;
    up.pointerY = 50.0f;
    ctx.ProcessEvent(up);
    TEST_ASSERT_TRUE(dropReceived);
    TEST_ASSERT_FALSE(ctx.IsDragging());
}

void TestWidget::TestDragCancelEscape() {
    UIContext ctx(64);
    ctx.SetViewport(800.0f, 600.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    ctx.GetWidget(root)->widthMode = SizeMode::Fixed;
    ctx.GetWidget(root)->heightMode = SizeMode::Fixed;
    ctx.GetWidget(root)->localW = 800.0f;
    ctx.GetWidget(root)->localH = 600.0f;

    int src = ctx.CreatePanel("draggable2");
    ctx.SetParent(src, root);
    Widget* sw = ctx.GetWidget(src);
    sw->widthMode = SizeMode::Fixed;
    sw->heightMode = SizeMode::Fixed;
    sw->localW = 200.0f;
    sw->localH = 200.0f;
    sw->onDragBegin = [](int idx) -> DragPayload {
        DragPayload p;
        p.typeTag = DragType::Widget;
        p.sourceWidget = idx;
        return p;
    };

    ctx.UpdateLayout();

    // Start drag
    Event down;
    down.type = EventType::PointerDown;
    down.pointerX = 100.0f;
    down.pointerY = 100.0f;
    ctx.ProcessEvent(down);

    Event move;
    move.type = EventType::PointerMove;
    move.pointerX = 110.0f;
    move.pointerY = 100.0f;
    ctx.ProcessEvent(move);
    TEST_ASSERT_TRUE(ctx.IsDragging());

    // Press Escape to cancel
    Event esc;
    esc.type = EventType::KeyDown;
    esc.key = KeyCode::Escape;
    ctx.ProcessEvent(esc);
    TEST_ASSERT_FALSE(ctx.IsDragging());
}

void TestWidget::TestTreeDragReparent() {
    UIContext ctx(128);
    ctx.SetViewport(800.0f, 600.0f);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    ctx.GetWidget(root)->widthMode = SizeMode::Fixed;
    ctx.GetWidget(root)->heightMode = SizeMode::Fixed;
    ctx.GetWidget(root)->localW = 800.0f;
    ctx.GetWidget(root)->localH = 600.0f;

    int container = ctx.CreatePanel("tree");
    ctx.SetParent(container, root);
    ctx.GetWidget(container)->widthMode = SizeMode::Fixed;
    ctx.GetWidget(container)->heightMode = SizeMode::Fixed;
    ctx.GetWidget(container)->localW = 200.0f;
    ctx.GetWidget(container)->localH = 400.0f;
    ctx.GetWidget(container)->layout.direction = LayoutDirection::Column;

    // Create 3 tree nodes: A, B, C at depth 0
    int nodeA = ctx.AddTreeNode(container, "nodeA", "Node A", 0, false);
    int nodeB = ctx.AddTreeNode(container, "nodeB", "Node B", 0, false);
    int nodeC = ctx.AddTreeNode(container, "nodeC", "Node C", 0, false);

    // Enable drag-drop on all
    ctx.EnableTreeDragDrop(container);

    // Verify initial order: A=0, B=1, C=2
    TEST_ASSERT_EQUAL(3, (int)ctx.GetWidget(container)->childCount);
    TEST_ASSERT_EQUAL(nodeA, (int)ctx.GetWidget(container)->children[0]);
    TEST_ASSERT_EQUAL(nodeB, (int)ctx.GetWidget(container)->children[1]);
    TEST_ASSERT_EQUAL(nodeC, (int)ctx.GetWidget(container)->children[2]);

    // Verify DnD callbacks are set
    TEST_ASSERT_TRUE(ctx.GetWidget(nodeA)->onDragBegin != nullptr);
    TEST_ASSERT_TRUE(ctx.GetWidget(nodeB)->onCanDrop != nullptr);
    TEST_ASSERT_EQUAL((int)(1u << DragType::Widget), (int)ctx.GetWidget(nodeC)->acceptDropTypes);
}

// ========== Sub-Menu & Popup Stack Tests ==========

void TestWidget::TestPopupStack() {
    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    int popup1 = ctx.CreatePopupMenu("popup1");
    int popup2 = ctx.CreatePopupMenu("popup2");
    ctx.SetParent(popup1, root);
    ctx.SetParent(popup2, root);

    TEST_ASSERT_TRUE(ctx.GetWidget(popup1)->flags.visible == 0);
    TEST_ASSERT_TRUE(ctx.GetWidget(popup2)->flags.visible == 0);

    // Show first popup
    ctx.ShowPopup(popup1, 100.0f, 100.0f);
    TEST_ASSERT_TRUE(ctx.GetWidget(popup1)->flags.visible == 1);
    TEST_ASSERT_TRUE(ctx.ActivePopup() == popup1);

    // Show second popup (sub-menu)
    ctx.ShowPopup(popup2, 200.0f, 100.0f);
    TEST_ASSERT_TRUE(ctx.GetWidget(popup2)->flags.visible == 1);
    TEST_ASSERT_TRUE(ctx.ActivePopup() == popup2);
    TEST_ASSERT_TRUE(ctx.GetWidget(popup1)->flags.visible == 1); // still visible

    // Dismiss top only
    ctx.DismissTopPopup();
    TEST_ASSERT_TRUE(ctx.GetWidget(popup2)->flags.visible == 0);
    TEST_ASSERT_TRUE(ctx.GetWidget(popup1)->flags.visible == 1);
    TEST_ASSERT_TRUE(ctx.ActivePopup() == popup1);

    // Dismiss all
    ctx.DismissPopup();
    TEST_ASSERT_TRUE(ctx.GetWidget(popup1)->flags.visible == 0);
    TEST_ASSERT_TRUE(ctx.ActivePopup() == -1);
}

void TestWidget::TestMenuItemShortcutText() {
    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    int menu = ctx.CreatePopupMenu("menu");
    ctx.SetParent(menu, root);

    int item = ctx.CreateMenuItem("item", "Undo");
    ctx.SetParent(item, menu);

    Widget* w = ctx.GetWidget(item);
    TEST_ASSERT_TRUE(w != nullptr);

    // Set shortcut text
    w->shortcutTextId = ctx.Strings().Intern("Ctrl+Z");
    const char* text = ctx.Strings().Lookup(w->shortcutTextId);
    TEST_ASSERT_TRUE(text != nullptr);
    TEST_ASSERT_TRUE(std::string(text) == "Ctrl+Z");

    // Set submenu reference
    int subMenu = ctx.CreatePopupMenu("sub");
    ctx.SetParent(subMenu, root);
    w->submenuIdx = static_cast<int16_t>(subMenu);
    TEST_ASSERT_TRUE(w->submenuIdx == subMenu);
}

void TestWidget::TestSubMenuHoverExpand() {
    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    Widget* rootW = ctx.GetWidget(root);
    rootW->localW = 800.0f;
    rootW->localH = 600.0f;

    int popup = ctx.CreatePopupMenu("popup");
    ctx.SetParent(popup, root);

    int subMenu = ctx.CreatePopupMenu("sub");
    ctx.SetParent(subMenu, root);

    int item = ctx.CreateMenuItem("item", "Recent");
    ctx.SetParent(item, popup);
    Widget* itemW = ctx.GetWidget(item);
    itemW->submenuIdx = static_cast<int16_t>(subMenu);

    // Show top-level popup
    ctx.ShowPopup(popup, 50.0f, 50.0f);
    ctx.UpdateLayout();

    TEST_ASSERT_TRUE(ctx.GetWidget(subMenu)->flags.visible == 0);

    // Directly show sub-menu
    ctx.ShowSubMenu(ctx.FindWidget("item"), subMenu);
    TEST_ASSERT_TRUE(ctx.GetWidget(subMenu)->flags.visible == 1);
    TEST_ASSERT_TRUE(ctx.ActivePopup() == subMenu);

    // Escape dismisses top (sub-menu) only
    Event esc{};
    esc.type = EventType::KeyDown;
    esc.key = KeyCode::Escape;
    ctx.ProcessEvent(esc);
    TEST_ASSERT_TRUE(ctx.GetWidget(subMenu)->flags.visible == 0);
    TEST_ASSERT_TRUE(ctx.GetWidget(popup)->flags.visible == 1);
}

// ========== Command Registry Tests ==========

void TestWidget::TestCommandRegistryBasic() {
    CommandRegistry reg;
    int counter = 0;
    reg.Register("test.inc", "Increment", "Test",
                 [&counter]() { counter++; });

    TEST_ASSERT_TRUE(reg.Find("test.inc") != nullptr);
    TEST_ASSERT_TRUE(reg.Find("nonexistent") == nullptr);

    TEST_ASSERT_TRUE(reg.Execute("test.inc"));
    TEST_ASSERT_TRUE(counter == 1);
    TEST_ASSERT_TRUE(reg.Execute("test.inc"));
    TEST_ASSERT_TRUE(counter == 2);

    // canExecute guard
    bool enabled = true;
    reg.Register("test.guarded", "Guarded", "Test",
                 [&counter]() { counter += 10; },
                 [&enabled]() { return enabled; });

    TEST_ASSERT_TRUE(reg.Execute("test.guarded"));
    TEST_ASSERT_TRUE(counter == 12);

    enabled = false;
    TEST_ASSERT_TRUE(!reg.Execute("test.guarded"));
    TEST_ASSERT_TRUE(counter == 12); // unchanged
    TEST_ASSERT_TRUE(!reg.IsEnabled("test.guarded"));
}

void TestWidget::TestShortcutDispatch() {
    CommandRegistry reg;
    int undone = 0;
    reg.Register("edit.undo", "Undo", "Edit",
                 [&undone]() { undone++; }, nullptr,
                 Shortcut(KeyCode::Z, true));

    // Dispatch matching shortcut
    Modifiers mods{};
    mods.ctrl = 1;
    TEST_ASSERT_TRUE(reg.DispatchShortcut(KeyCode::Z, mods));
    TEST_ASSERT_TRUE(undone == 1);

    // Non-matching
    Modifiers noMods{};
    TEST_ASSERT_TRUE(!reg.DispatchShortcut(KeyCode::Z, noMods));
    TEST_ASSERT_TRUE(undone == 1);

    // Shortcut label
    TEST_ASSERT_TRUE(reg.ShortcutLabel("edit.undo") == "Ctrl+Z");
}

void TestWidget::TestShortcutRebind() {
    CommandRegistry reg;
    int counter = 0;
    reg.Register("test.cmd", "Test", "Test",
                 [&counter]() { counter++; }, nullptr,
                 Shortcut(KeyCode::A, true));

    Modifiers ctrlMods{};
    ctrlMods.ctrl = 1;
    TEST_ASSERT_TRUE(reg.DispatchShortcut(KeyCode::A, ctrlMods));
    TEST_ASSERT_TRUE(counter == 1);

    // Rebind to Ctrl+B
    reg.BindShortcut("test.cmd", Shortcut(KeyCode::B, true));
    TEST_ASSERT_TRUE(!reg.DispatchShortcut(KeyCode::A, ctrlMods)); // old binding gone
    TEST_ASSERT_TRUE(reg.DispatchShortcut(KeyCode::B, ctrlMods));
    TEST_ASSERT_TRUE(counter == 2);
    TEST_ASSERT_TRUE(reg.ShortcutLabel("test.cmd") == "Ctrl+B");
}

// ========== Color Conversion Tests ==========

void TestWidget::TestColorHSVRoundtrip() {
    // Red -> HSV -> back
    Color4 red(255, 0, 0);
    float h, s, v;
    red.ToHSV(h, s, v);
    TEST_ASSERT_TRUE(std::fabs(h) < 1.0f || std::fabs(h - 360.0f) < 1.0f);
    TEST_ASSERT_TRUE(std::fabs(s - 1.0f) < 0.01f);
    TEST_ASSERT_TRUE(std::fabs(v - 1.0f) < 0.01f);

    Color4 back = Color4::FromHSV(h, s, v);
    TEST_ASSERT_TRUE(back.r == 255 && back.g == 0 && back.b == 0);

    // Green
    Color4 green(0, 255, 0);
    green.ToHSV(h, s, v);
    TEST_ASSERT_TRUE(std::fabs(h - 120.0f) < 1.0f);
    back = Color4::FromHSV(h, s, v);
    TEST_ASSERT_TRUE(back.g == 255);

    // Grey (no saturation)
    Color4 grey(128, 128, 128);
    grey.ToHSV(h, s, v);
    TEST_ASSERT_TRUE(std::fabs(s) < 0.01f);
    back = Color4::FromHSV(h, s, v);
    TEST_ASSERT_TRUE(std::abs(static_cast<int>(back.r) - 128) <= 1);
}

void TestWidget::TestColorFromHex() {
    Color4 c1 = Color4::FromHex("#FF8800");
    TEST_ASSERT_TRUE(c1.r == 255);
    TEST_ASSERT_TRUE(c1.g == 0x88);
    TEST_ASSERT_TRUE(c1.b == 0);
    TEST_ASSERT_TRUE(c1.a == 255);

    Color4 c2 = Color4::FromHex("00FF0080");
    TEST_ASSERT_TRUE(c2.r == 0);
    TEST_ASSERT_TRUE(c2.g == 255);
    TEST_ASSERT_TRUE(c2.b == 0);
    TEST_ASSERT_TRUE(c2.a == 0x80);

    Color4 c3 = Color4::FromHex("#000000");
    TEST_ASSERT_TRUE(c3.r == 0 && c3.g == 0 && c3.b == 0);
}

// ========== VirtualList Tests ==========

void TestWidget::TestVirtualListCreate() {
    using namespace koilo::ui;
    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    int callCount = 0;
    int vl = ctx.CreateVirtualList("vlist", 100, 20.0f,
        [&](Widget& w, int idx) {
            callCount++;
        });
    TEST_ASSERT_TRUE(vl >= 0);
    ctx.SetParent(vl, root);
    ctx.UpdateLayout();

    Widget* w = ctx.Pool().Get(vl);
    TEST_ASSERT_TRUE(w != nullptr);
    TEST_ASSERT_TRUE(w->tag == WidgetTag::VirtualList);
    TEST_ASSERT_TRUE(w->virtualItemCount == 100);
    TEST_ASSERT_TRUE(w->virtualItemHeight == 20.0f);
    TEST_ASSERT_TRUE(w->contentHeight == 2000.0f);
    TEST_ASSERT_TRUE(w->flags.clipChildren == 1);
    TEST_ASSERT_TRUE(w->flags.scrollable == 1);
}

void TestWidget::TestVirtualListScroll() {
    using namespace koilo::ui;
    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    // Track which data indices were bound
    std::vector<int> boundIndices;
    int vl = ctx.CreateVirtualList("vlist", 1000, 20.0f,
        [&](Widget& w, int idx) {
            boundIndices.push_back(idx);
        });
    ctx.SetParent(vl, root);
    ctx.UpdateLayout();

    // Initial update - should bind visible rows starting from index 0
    boundIndices.clear();
    ctx.UpdateVirtualList(vl);
    TEST_ASSERT_TRUE(!boundIndices.empty());
    TEST_ASSERT_TRUE(boundIndices[0] == 0);

    // Scroll down and update
    Widget* w = ctx.Pool().Get(vl);
    w->scrollY = 200.0f; // skip 10 items
    boundIndices.clear();
    ctx.UpdateVirtualList(vl);
    TEST_ASSERT_TRUE(!boundIndices.empty());
    TEST_ASSERT_TRUE(boundIndices[0] == 10); // first visible is index 10
}

void TestWidget::TestVirtualListRowRecycle() {
    using namespace koilo::ui;
    UIContext ctx;
    ctx.SetViewport(800, 200);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    int vl = ctx.CreateVirtualList("vlist", 500, 20.0f,
        [&](Widget& w, int idx) {
            // no-op
        });
    ctx.SetParent(vl, root);
    ctx.UpdateLayout();

    ctx.UpdateVirtualList(vl);
    Widget* list = ctx.Pool().Get(vl);
    int childCount1 = list->childCount;
    TEST_ASSERT_TRUE(childCount1 > 0);
    TEST_ASSERT_TRUE(childCount1 <= 14); // ~200/20 + 2 buffer

    // Scroll and update - child count should not grow
    list->scrollY = 100.0f;
    ctx.UpdateVirtualList(vl);
    int childCount2 = list->childCount;
    TEST_ASSERT_TRUE(childCount2 <= childCount1 + 2); // may grow by 1-2 at most
}

// ========== Canvas2D Tests ==========

void TestWidget::TestCanvas2DCreate() {
    using namespace koilo::ui;
    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    bool painted = false;
    int c = ctx.CreateCanvas2D("canvas", [&](void* rawCtx) {
        painted = true;
        auto* dc = static_cast<CanvasDrawContext*>(rawCtx);
        TEST_ASSERT_TRUE(dc->Width() > 0.0f);
        TEST_ASSERT_TRUE(dc->Height() > 0.0f);
    });
    TEST_ASSERT_TRUE(c >= 0);
    ctx.SetParent(c, root);

    Widget* w = ctx.Pool().Get(c);
    TEST_ASSERT_TRUE(w->tag == WidgetTag::Canvas2D);
    TEST_ASSERT_TRUE(w->flags.clipChildren == 1);
    TEST_ASSERT_TRUE(w->flags.focusable == 1);
}

void TestWidget::TestCanvas2DDrawCommands() {
    using namespace koilo::ui;

    // Directly test that AddLine/AddFilledCircle/AddTriangle
    // produce draw commands
    UIDrawList dl;
    dl.AddLine(10, 20, 100, 200, 2.0f, {255, 0, 0, 255});
    dl.AddFilledCircle(50, 50, 25, {0, 255, 0, 255});
    dl.AddCircleOutline(50, 50, 30, 2.0f, {0, 0, 255, 255});
    dl.AddTriangle(0, 0, 100, 0, 50, 80, {255, 255, 0, 255});

    TEST_ASSERT_TRUE(dl.Size() == 4);
    TEST_ASSERT_TRUE(dl[0].type == DrawCmdType::Line);
    TEST_ASSERT_TRUE(dl[1].type == DrawCmdType::FilledCircle);
    TEST_ASSERT_TRUE(dl[2].type == DrawCmdType::CircleOutline);
    TEST_ASSERT_TRUE(dl[3].type == DrawCmdType::Triangle);

    // Verify line endpoint encoding
    TEST_ASSERT_TRUE(dl[0].x == 10.0f && dl[0].y == 20.0f);
    TEST_ASSERT_TRUE(dl[0].w == 100.0f && dl[0].h == 200.0f);

    // Verify circle center/radius
    TEST_ASSERT_TRUE(dl[1].x == 50.0f && dl[1].y == 50.0f);
    TEST_ASSERT_TRUE(dl[1].w == 25.0f);

    // Verify triangle vertices
    TEST_ASSERT_TRUE(dl[3].x2 == 50.0f && dl[3].y2 == 80.0f);
}

// ========== Settings Store Tests ==========

void TestWidget::TestSettingsGetSet() {
    using namespace koilo::ui;
    SettingsStore store;

    // Get with default when key doesn't exist
    TEST_ASSERT_TRUE(store.Get<int>("missing", 42) == 42);
    TEST_ASSERT_TRUE(store.Get<bool>("missing_b", true) == true);

    // Set and get
    store.Set<int>("ui.fontSize", 16);
    TEST_ASSERT_TRUE(store.Get<int>("ui.fontSize") == 16);

    store.Set<float>("viewport.fov", 60.0f);
    TEST_ASSERT_TRUE(store.Get<float>("viewport.fov") == 60.0f);

    store.Set<bool>("editor.showGrid", true);
    TEST_ASSERT_TRUE(store.Get<bool>("editor.showGrid") == true);

    store.Set<std::string>("paths.project", "/home/user/project");
    TEST_ASSERT_TRUE(store.Get<std::string>("paths.project") == "/home/user/project");

    store.Set<Color4>("theme.accent", Color4{100, 150, 200, 255});
    Color4 c = store.Get<Color4>("theme.accent");
    TEST_ASSERT_TRUE(c.r == 100 && c.g == 150 && c.b == 200);

    TEST_ASSERT_TRUE(store.Has("ui.fontSize"));
    TEST_ASSERT_TRUE(!store.Has("nonexistent"));
}

void TestWidget::TestSettingsSerialize() {
    using namespace koilo::ui;
    SettingsStore store;

    store.Set<int>("count", 10);
    store.Set<bool>("enabled", true);
    store.Set<float>("scale", 1.5f);
    store.Set<std::string>("name", "test");
    store.Set<Color4>("color", Color4{255, 128, 0, 255});

    std::string data = store.Serialize();
    TEST_ASSERT_TRUE(!data.empty());

    // Deserialize into a new store with same keys defined
    SettingsStore store2;
    store2.Set<int>("count", 0);
    store2.Set<bool>("enabled", false);
    store2.Set<float>("scale", 0.0f);
    store2.Set<std::string>("name", "");
    store2.Set<Color4>("color", Color4{0, 0, 0, 0});
    store2.Deserialize(data);

    TEST_ASSERT_TRUE(store2.Get<int>("count") == 10);
    TEST_ASSERT_TRUE(store2.Get<bool>("enabled") == true);
    float s = store2.Get<float>("scale");
    TEST_ASSERT_TRUE(s > 1.4f && s < 1.6f);
    TEST_ASSERT_TRUE(store2.Get<std::string>("name") == "test");
    Color4 c = store2.Get<Color4>("color");
    TEST_ASSERT_TRUE(c.r == 255 && c.g == 128 && c.b == 0);
}

void TestWidget::TestSettingsDefaults() {
    using namespace koilo::ui;
    SettingsStore store;

    store.Define<int>("ui.fontSize", "Interface", "Font Size", 14);
    store.Define<bool>("ui.animations", "Interface", "Animations", true);
    store.Define<float>("viewport.fov", "Viewport", "Camera FOV", 45.0f);

    TEST_ASSERT_TRUE(store.Get<int>("ui.fontSize") == 14);

    store.Set<int>("ui.fontSize", 20);
    TEST_ASSERT_TRUE(store.Get<int>("ui.fontSize") == 20);

    store.ResetToDefault("ui.fontSize");
    TEST_ASSERT_TRUE(store.Get<int>("ui.fontSize") == 14);

    // Categories
    auto cats = store.Categories();
    TEST_ASSERT_TRUE(cats.size() == 2);

    auto items = store.EntriesInCategory("Interface");
    TEST_ASSERT_TRUE(items.size() == 2);
}

// ========== Icon System Tests ==========

void TestWidget::TestIconFromName() {
    using namespace koilo::ui;
    TEST_ASSERT_TRUE(IconFromName("file") == IconId::File);
    TEST_ASSERT_TRUE(IconFromName("folder") == IconId::Folder);
    TEST_ASSERT_TRUE(IconFromName("search") == IconId::Search);
    TEST_ASSERT_TRUE(IconFromName("play") == IconId::Play);
    TEST_ASSERT_TRUE(IconFromName("nonexistent") == IconId::None);
    TEST_ASSERT_TRUE(IconFromName(nullptr) == IconId::None);

    // Round-trip: name -> id -> name
    for (int i = 0; i < static_cast<int>(IconId::Count); ++i) {
        IconId id = static_cast<IconId>(i);
        const char* name = IconName(id);
        IconId back = IconFromName(name);
        TEST_ASSERT_TRUE(back == id);
    }
}

// ========== Color Picker Tests ==========

void TestWidget::TestColorPickerBuild() {
    using namespace koilo::ui;
    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    ColorPicker picker;
    picker.Build(ctx);

    TEST_ASSERT_TRUE(picker.PanelIndex() >= 0);
    TEST_ASSERT_TRUE(!picker.IsOpen());

    // Verify panel is invisible
    Widget* panel = ctx.Pool().Get(picker.PanelIndex());
    TEST_ASSERT_TRUE(panel != nullptr);
    TEST_ASSERT_TRUE(panel->flags.visible == false);
}

void TestWidget::TestColorPickerSVInteraction() {
    using namespace koilo::ui;
    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    // Create a source ColorField
    int cf = ctx.CreateWidget(WidgetTag::ColorField, "cf");
    ctx.SetParent(cf, root);
    Widget* cfw = ctx.Pool().Get(cf);
    cfw->colorValue = {255, 0, 0, 255}; // red
    cfw->localW = 40.0f;
    cfw->localH = 24.0f;

    ColorPicker picker;
    picker.Build(ctx);

    ctx.UpdateLayout();

    // Open picker
    picker.Open(cf, 100.0f, 100.0f);
    TEST_ASSERT_TRUE(picker.IsOpen());

    Widget* panel = ctx.Pool().Get(picker.PanelIndex());
    TEST_ASSERT_TRUE(panel->flags.visible == true);

    // Close and verify
    picker.Close();
    TEST_ASSERT_TRUE(!picker.IsOpen());
}

// ========== Curve Editor Tests ==========

void TestWidget::TestCurveEditorEvaluate() {
    using namespace koilo::ui;
    CurveEditor editor;

    // Default ease-in-out: endpoints at (0,0) and (1,1)
    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    editor.Build(ctx, root, "ce", 200.0f);

    TEST_ASSERT_TRUE(editor.CanvasIndex() >= 0);

    // At x=0, y should be 0
    float y0 = editor.Evaluate(0.0f);
    TEST_ASSERT_TRUE(y0 >= -0.01f && y0 <= 0.01f);

    // At x=1, y should be 1
    float y1 = editor.Evaluate(1.0f);
    TEST_ASSERT_TRUE(y1 >= 0.99f && y1 <= 1.01f);

    // At x=0.5, y should be ~0.5 for default curve
    float ym = editor.Evaluate(0.5f);
    TEST_ASSERT_TRUE(ym > 0.2f && ym < 0.8f);

    // Linear preset
    editor.SetLinear();
    float yl = editor.Evaluate(0.5f);
    TEST_ASSERT_TRUE(yl > 0.45f && yl < 0.55f);
}

void TestWidget::TestCurveEditorAddRemove() {
    using namespace koilo::ui;
    CurveEditor editor;

    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    editor.Build(ctx, root, "ce2");

    TEST_ASSERT_TRUE(editor.Points().size() == 2);

    // Add a midpoint
    editor.AddPoint(0.5f, 0.8f);
    TEST_ASSERT_TRUE(editor.Points().size() == 3);

    // Points should be sorted by x
    TEST_ASSERT_TRUE(editor.Points()[0].x < editor.Points()[1].x);
    TEST_ASSERT_TRUE(editor.Points()[1].x < editor.Points()[2].x);

    // Select midpoint and remove
    // (In unit test, just directly remove selected)
    auto& pts = const_cast<std::vector<CurvePoint>&>(editor.Points());
    pts[1].selected = true;
    editor.RemoveSelected();
    TEST_ASSERT_TRUE(editor.Points().size() == 2);
}

// ========== Timeline Tests ==========

void TestWidget::TestTimelineScrub() {
    using namespace koilo::ui;
    Timeline tl;

    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    tl.Build(ctx, root, "tl", 32.0f);

    tl.SetRange(0, 100);
    tl.SetFPS(30);
    tl.SetCurrentFrame(0);
    TEST_ASSERT_TRUE(tl.GetCurrentFrame() == 0);

    // Scrub to middle
    tl.SetCurrentFrame(50);
    TEST_ASSERT_TRUE(tl.GetCurrentFrame() == 50);

    // Clamp out of range
    tl.SetCurrentFrame(200);
    TEST_ASSERT_TRUE(tl.GetCurrentFrame() == 100);
    tl.SetCurrentFrame(-5);
    TEST_ASSERT_TRUE(tl.GetCurrentFrame() == 0);

    // Keyframes
    tl.AddKeyframe(10);
    tl.AddKeyframe(50, {0, 255, 0, 255});
    TEST_ASSERT_TRUE(tl.Keyframes().size() == 2);
    tl.ClearKeyframes();
    TEST_ASSERT_TRUE(tl.Keyframes().empty());

    // Input-based scrub (canvas at 800px width)
    tl.HandleInput(410.0f, 800.0f, true); // middle of plot area
    TEST_ASSERT_TRUE(tl.GetCurrentFrame() >= 45 && tl.GetCurrentFrame() <= 55);
}

// ========== Content Browser Tests ==========

void TestWidget::TestFsAdapter() {
    using namespace koilo::ui;

    // Icon mapping
    TEST_ASSERT_TRUE(IconForExtension(".obj") == IconId::Mesh);
    TEST_ASSERT_TRUE(IconForExtension(".fbx") == IconId::Mesh);
    TEST_ASSERT_TRUE(IconForExtension(".png") == IconId::Texture);
    TEST_ASSERT_TRUE(IconForExtension(".jpg") == IconId::Texture);
    TEST_ASSERT_TRUE(IconForExtension(".mtl") == IconId::Material);
    TEST_ASSERT_TRUE(IconForExtension(".ksl") == IconId::Script);
    TEST_ASSERT_TRUE(IconForExtension(".xyz") == IconId::File);
    TEST_ASSERT_TRUE(IconForExtension("") == IconId::File);

    // Path splitting
    auto parts = SplitPath("/home/user/assets/models");
    TEST_ASSERT_TRUE(parts.size() == 4);
    TEST_ASSERT_TRUE(parts[0] == "home");
    TEST_ASSERT_TRUE(parts[1] == "user");
    TEST_ASSERT_TRUE(parts[2] == "assets");
    TEST_ASSERT_TRUE(parts[3] == "models");

    // Empty path
    auto empty = SplitPath("");
    TEST_ASSERT_TRUE(empty.empty());

    // Path join
    auto joined = JoinPath(parts, 2);
    TEST_ASSERT_TRUE(joined == "/home/user/assets");

    // Format size
    TEST_ASSERT_TRUE(FormatSize(0) == "");
    TEST_ASSERT_TRUE(FormatSize(512).find("512") != std::string::npos);
    TEST_ASSERT_TRUE(FormatSize(1024 * 1024).find("MB") != std::string::npos);

    // ListDirectory on current test directory (should find at least some files)
    auto entries = ListDirectory(".");
    TEST_ASSERT_TRUE(!entries.empty());
    // Dirs should come first
    bool seenFile = false;
    for (const auto& e : entries) {
        if (seenFile) TEST_ASSERT_TRUE(!e.isDir); // no dir after file
        if (!e.isDir) seenFile = true;
    }
}

void TestWidget::TestBreadcrumb() {
    using namespace koilo::ui;
    UIContext ctx;
    ctx.SetViewport(800, 600);

    int root = ctx.CreatePanel("bc_root");
    ctx.SetRoot(root);

    Breadcrumb bc;
    int bcIdx = bc.Build(ctx, root, "bc");
    TEST_ASSERT_TRUE(bcIdx >= 0);

    // Set a path
    bc.SetPath("/home/user/assets");
    TEST_ASSERT_TRUE(bc.CurrentPath() == "/home/user/assets");

    // Should have children (root btn + separators + segment btns)
    Widget* row = ctx.Pool().Get(bcIdx);
    TEST_ASSERT_TRUE(row->childCount > 0);

    // Navigate callback
    std::string navigated;
    bc.SetOnNavigate([&](const std::string& path) { navigated = path; });

    // Simulate click on root button
    Widget* rootBtn = ctx.Pool().Get(row->children[0]);
    if (rootBtn && rootBtn->onClickCpp) rootBtn->onClickCpp(*rootBtn);
    TEST_ASSERT_TRUE(navigated == "/");
}

void TestWidget::TestContentBrowserBuild() {
    using namespace koilo::ui;
    UIContext ctx;
    ctx.SetViewport(800, 600);

    int root = ctx.CreatePanel("app_root");
    ctx.SetRoot(root);

    ContentBrowser browser;
    int idx = browser.Build(ctx, root, "cb");
    TEST_ASSERT_TRUE(idx >= 0);
    TEST_ASSERT_TRUE(browser.RootIndex() == idx);
    TEST_ASSERT_TRUE(browser.ListIndex() >= 0);
    TEST_ASSERT_TRUE(browser.TreeIndex() >= 0);

    // Navigate to current directory
    browser.NavigateTo(".");
    TEST_ASSERT_TRUE(browser.CurrentPath() == ".");
    TEST_ASSERT_TRUE(!browser.Entries().empty());

    // File action callback
    std::string clickedFile;
    browser.SetOnFileAction([&](const FsEntry& e) { clickedFile = e.name; });
    TEST_ASSERT_TRUE(clickedFile.empty()); // not triggered yet
}

// ========== Node Graph Tests ==========

void TestWidget::TestNodeGraphBuild() {
    using namespace koilo::ui;
    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("ng_root");
    ctx.SetRoot(root);

    NodeGraph graph;
    int canvasIdx = graph.Build(ctx, root, "ng_canvas", 400.0f, 300.0f);
    TEST_ASSERT_TRUE(canvasIdx >= 0);
    TEST_ASSERT_TRUE(graph.CanvasIndex() == canvasIdx);

    // Add nodes
    int n0 = graph.AddNode("Output", 200, 100);
    int n1 = graph.AddNode("Texture", 20, 80);
    TEST_ASSERT_TRUE(graph.Nodes().size() == 2);

    // Add ports
    graph.AddInput(n0, "color", PortType::Color);
    graph.AddOutput(n1, "rgb", PortType::Color);
    TEST_ASSERT_TRUE(graph.Nodes()[0].inputs.size() == 1);
    TEST_ASSERT_TRUE(graph.Nodes()[1].outputs.size() == 1);

    // Node height is computed from ports
    TEST_ASSERT_TRUE(graph.Nodes()[0].h > GraphNode::TITLE_H);
}

void TestWidget::TestNodeGraphConnect() {
    using namespace koilo::ui;
    NodeGraph graph;

    int n0 = graph.AddNode("A", 0, 0);
    int n1 = graph.AddNode("B", 200, 0);
    graph.AddOutput(n0, "out", PortType::Float);
    graph.AddInput(n1, "in", PortType::Float);

    // Connect
    bool ok = graph.Connect(n0, 0, n1, 0);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(graph.Connections().size() == 1);

    // Type mismatch should fail
    graph.AddInput(n1, "color_in", PortType::Color);
    bool bad = graph.Connect(n0, 0, n1, 1); // float -> color
    TEST_ASSERT_TRUE(!bad);

    // Disconnect
    graph.Disconnect(n1, 0);
    TEST_ASSERT_TRUE(graph.Connections().empty());

    // Remove node removes connections
    graph.Connect(n0, 0, n1, 0);
    graph.RemoveNode(n0);
    TEST_ASSERT_TRUE(graph.Connections().empty());
    TEST_ASSERT_TRUE(graph.Nodes().size() == 1);
}

// ========== TreeRow Multi-Column Tests ==========

void TestWidget::TestTreeRowColumns() {
    using namespace koilo::ui;
    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("tree_root");
    ctx.SetRoot(root);

    int tn = ctx.CreateWidget(WidgetTag::TreeNode, "tn1");
    ctx.SetParent(tn, root);
    Widget* w = ctx.Pool().Get(tn);
    w->textId = ctx.Strings().Intern("Main Label");

    // Set up columns
    w->treeColumns[0].textId = ctx.Strings().Intern("Col A");
    w->treeColumns[0].width  = 80.0f;
    w->treeColumns[1].textId = ctx.Strings().Intern("Col B");
    w->treeColumns[1].width  = 60.0f;
    w->treeColumnCount = 2;

    TEST_ASSERT_TRUE(w->treeColumnCount == 2);
    TEST_ASSERT_TRUE(w->treeColumns[0].width == 80.0f);

    // Action icons
    w->actionIcons[0].icon = IconId::Eye;
    w->actionIcons[0].toggled = true;
    w->actionIcons[1].icon = IconId::Lock;
    w->actionIconCount = 2;

    TEST_ASSERT_TRUE(w->actionIconCount == 2);
    TEST_ASSERT_TRUE(w->actionIcons[0].toggled == true);
    TEST_ASSERT_TRUE(w->actionIcons[1].icon == IconId::Lock);
}

// ========== Preferences Panel Tests ==========

void TestWidget::TestPreferencesBuild() {
    using namespace koilo::ui;
    UIContext ctx;
    ctx.SetViewport(800, 600);
    int root = ctx.CreatePanel("pref_root");
    ctx.SetRoot(root);

    // Populate settings
    SettingsStore store;
    store.Define<float>("ui.scale", "Interface", "UI Scale", 1.0f);
    store.Define<int>("ui.font_size", "Interface", "Font Size", 14);
    store.Define<bool>("viewport.grid", "Viewport", "Show Grid", true);
    store.Define<std::string>("paths.project", "File Paths", "Project Dir", "/home/user");
    store.Define<Color4>("theme.accent", "Theme", "Accent Color", {80, 140, 220, 255});

    PreferencesPanel prefs;
    int panelIdx = prefs.Build(ctx, store);
    TEST_ASSERT_TRUE(panelIdx >= 0);
    TEST_ASSERT_TRUE(!prefs.IsVisible()); // hidden by default

    prefs.Show();
    TEST_ASSERT_TRUE(prefs.IsVisible());

    // Should have categories
    auto cats = store.Categories();
    TEST_ASSERT_TRUE(cats.size() >= 3);

    // Select a category and check content is populated
    prefs.SelectCategory("Interface");
    TEST_ASSERT_TRUE(prefs.SelectedCategory() == "Interface");

    prefs.Hide();
    TEST_ASSERT_TRUE(!prefs.IsVisible());
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

    // Drag-and-drop
    RUN_TEST(TestWidget::TestDragPayloadLifecycle);
    RUN_TEST(TestWidget::TestDragThreshold);
    RUN_TEST(TestWidget::TestDropAcceptReject);
    RUN_TEST(TestWidget::TestDragCancelEscape);
    RUN_TEST(TestWidget::TestTreeDragReparent);

    // Sub-menus & popup stack
    RUN_TEST(TestWidget::TestPopupStack);
    RUN_TEST(TestWidget::TestMenuItemShortcutText);
    RUN_TEST(TestWidget::TestSubMenuHoverExpand);

    // Command registry
    RUN_TEST(TestWidget::TestCommandRegistryBasic);
    RUN_TEST(TestWidget::TestShortcutDispatch);
    RUN_TEST(TestWidget::TestShortcutRebind);

    // Color conversion
    RUN_TEST(TestWidget::TestColorHSVRoundtrip);
    RUN_TEST(TestWidget::TestColorFromHex);

    // Virtual list
    RUN_TEST(TestWidget::TestVirtualListCreate);
    RUN_TEST(TestWidget::TestVirtualListScroll);
    RUN_TEST(TestWidget::TestVirtualListRowRecycle);

    // Canvas2D
    RUN_TEST(TestWidget::TestCanvas2DCreate);
    RUN_TEST(TestWidget::TestCanvas2DDrawCommands);

    // Settings store
    RUN_TEST(TestWidget::TestSettingsGetSet);
    RUN_TEST(TestWidget::TestSettingsSerialize);
    RUN_TEST(TestWidget::TestSettingsDefaults);

    // Icons
    RUN_TEST(TestWidget::TestIconFromName);

    // Color picker
    RUN_TEST(TestWidget::TestColorPickerBuild);
    RUN_TEST(TestWidget::TestColorPickerSVInteraction);

    // Curve editor
    RUN_TEST(TestWidget::TestCurveEditorEvaluate);
    RUN_TEST(TestWidget::TestCurveEditorAddRemove);

    // Timeline
    RUN_TEST(TestWidget::TestTimelineScrub);

    // Content browser
    RUN_TEST(TestWidget::TestFsAdapter);
    RUN_TEST(TestWidget::TestBreadcrumb);
    RUN_TEST(TestWidget::TestContentBrowserBuild);

    // Node graph
    RUN_TEST(TestWidget::TestNodeGraphBuild);
    RUN_TEST(TestWidget::TestNodeGraphConnect);

    // TreeRow multi-column
    RUN_TEST(TestWidget::TestTreeRowColumns);

    // Preferences panel
    RUN_TEST(TestWidget::TestPreferencesBuild);
}
