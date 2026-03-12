// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testautoinspector.cpp
 * @brief Unit tests for the auto-inspector generator.
 */

#include "testautoinspector.hpp"
#include <koilo/systems/ui/auto_inspector.hpp>
#include <koilo/registry/reflect_macros.hpp>

using namespace koilo;
using namespace koilo::ui;

// -- Test fixture struct with reflection ------------------------------

struct InspTestObj {
    float speed = 5.0f;
    int   count = 10;
    bool  active = true;
    float hidden_val = 0.0f;
    float readonly_val = 42.0f;
    float angle = 90.0f;
    float opacity = 0.5f;

    KL_BEGIN_FIELDS(InspTestObj)
        KL_FIELD(InspTestObj, speed,   "Movement speed", 0.0f, 100.0f),
        KL_FIELD(InspTestObj, count,   "Item count",     0, 0),
        KL_FIELD(InspTestObj, active,  "Is active",      0, 0),
        KL_FIELD_EX(InspTestObj, hidden_val, "Internal", 0, 0, Hidden, nullptr),
        KL_FIELD_EX(InspTestObj, readonly_val, "Read only value", 0, 0, ReadOnly, nullptr),
        KL_FIELD_EX(InspTestObj, angle, "Rotation angle", 0, 360, Angle, "Transform"),
        KL_FIELD_EX(InspTestObj, opacity, "Opacity", 0, 1, Normalized, "Rendering")
    KL_END_FIELDS

    static koilo::MethodList Methods() { return {nullptr, 0}; }

    KL_BEGIN_DESCRIBE(InspTestObj)
        KL_CTOR0(InspTestObj)
    KL_END_DESCRIBE(InspTestObj)
};

// -- Tests ------------------------------------------------------------

void TestAutoInspector::TestNullDesc() {
    UIContext ctx;
    InspectorResult r = GenerateInspector(nullptr, nullptr, ctx);
    TEST_ASSERT_EQUAL(-1, r.rootWidget);
    TEST_ASSERT_EQUAL(0, r.fieldCount);
}

void TestAutoInspector::TestGenerateEmpty() {
    // A ClassDesc with zero fields
    static const ClassDesc empty{"Empty", {nullptr, 0}, {nullptr, 0},
                                  nullptr, 0, 0, nullptr, nullptr};
    int dummy = 0;
    UIContext ctx;
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);
    InspectorResult r = GenerateInspector(&empty, &dummy, ctx, root);
    TEST_ASSERT(r.rootWidget >= 0);
    TEST_ASSERT_EQUAL(0, r.fieldCount);
}

void TestAutoInspector::TestGenerateFloat() {
    InspTestObj obj;
    const ClassDesc& desc = InspTestObj::Describe();
    UIContext ctx;
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    InspectorResult r = GenerateInspector(&desc, &obj, ctx, root);
    TEST_ASSERT(r.rootWidget >= 0);
    // speed (slider) + count (label) + active (checkbox) + readonly_val (label)
    // + angle (slider) + opacity (slider) = 6 (hidden excluded)
    TEST_ASSERT_EQUAL(6, r.fieldCount);

    // Speed field should create a slider (has min/max)
    int sld = ctx.FindWidget("insp_InspTestObj_speed_sld");
    TEST_ASSERT(sld >= 0);
    Widget* w = ctx.GetWidget(sld);
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_EQUAL(WidgetTag::Slider, w->tag);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, w->sliderValue);
}

void TestAutoInspector::TestGenerateIntSlider() {
    InspTestObj obj;
    obj.count = 7;
    const ClassDesc& desc = InspTestObj::Describe();
    UIContext ctx;
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    GenerateInspector(&desc, &obj, ctx, root);

    // Count has no range -> displayed as label, not slider
    int val = ctx.FindWidget("insp_InspTestObj_count_val");
    TEST_ASSERT(val >= 0);
}

void TestAutoInspector::TestGenerateBool() {
    InspTestObj obj;
    const ClassDesc& desc = InspTestObj::Describe();
    UIContext ctx;
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    GenerateInspector(&desc, &obj, ctx, root);

    int chk = ctx.FindWidget("insp_InspTestObj_active_chk");
    TEST_ASSERT(chk >= 0);
    Widget* w = ctx.GetWidget(chk);
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_EQUAL(WidgetTag::Checkbox, w->tag);
    TEST_ASSERT_TRUE(w->checked);
}

void TestAutoInspector::TestSliderBinding() {
    InspTestObj obj;
    obj.speed = 10.0f;
    const ClassDesc& desc = InspTestObj::Describe();
    UIContext ctx;
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    GenerateInspector(&desc, &obj, ctx, root);

    int sld = ctx.FindWidget("insp_InspTestObj_speed_sld");
    TEST_ASSERT(sld >= 0);
    Widget* w = ctx.GetWidget(sld);
    TEST_ASSERT_NOT_NULL(w);

    // Simulate slider change
    w->sliderValue = 75.0f;
    if (w->onChangeCpp) w->onChangeCpp(*w);

    // Verify write-back to instance
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 75.0f, obj.speed);
}

void TestAutoInspector::TestCheckboxBinding() {
    InspTestObj obj;
    obj.active = false;
    const ClassDesc& desc = InspTestObj::Describe();
    UIContext ctx;
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    GenerateInspector(&desc, &obj, ctx, root);

    int chk = ctx.FindWidget("insp_InspTestObj_active_chk");
    TEST_ASSERT(chk >= 0);
    Widget* w = ctx.GetWidget(chk);
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_FALSE(w->checked);

    // Simulate checkbox toggle
    w->checked = true;
    if (w->onChangeCpp) w->onChangeCpp(*w);

    TEST_ASSERT_TRUE(obj.active);
}

void TestAutoInspector::TestHiddenField() {
    InspTestObj obj;
    const ClassDesc& desc = InspTestObj::Describe();
    UIContext ctx;
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    GenerateInspector(&desc, &obj, ctx, root);

    // hidden_val should not have any widget
    int found = ctx.FindWidget("insp_InspTestObj_hidden_val_sld");
    TEST_ASSERT_EQUAL(-1, found);
    found = ctx.FindWidget("insp_InspTestObj_hidden_val_val");
    TEST_ASSERT_EQUAL(-1, found);
    found = ctx.FindWidget("insp_InspTestObj_hidden_val_chk");
    TEST_ASSERT_EQUAL(-1, found);
}

void TestAutoInspector::TestReadOnlyField() {
    InspTestObj obj;
    const ClassDesc& desc = InspTestObj::Describe();
    UIContext ctx;
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    GenerateInspector(&desc, &obj, ctx, root);

    // readonly_val should exist but be disabled
    int val = ctx.FindWidget("insp_InspTestObj_readonly_val_val");
    TEST_ASSERT(val >= 0);
    Widget* w = ctx.GetWidget(val);
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_EQUAL(0, w->flags.enabled);
}

void TestAutoInspector::TestCategoryGrouping() {
    InspTestObj obj;
    const ClassDesc& desc = InspTestObj::Describe();
    UIContext ctx;
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    GenerateInspector(&desc, &obj, ctx, root);

    // Should have category panels
    int transformCat = ctx.FindWidget("insp_InspTestObj_cat_Transform");
    TEST_ASSERT(transformCat >= 0);

    int renderCat = ctx.FindWidget("insp_InspTestObj_cat_Rendering");
    TEST_ASSERT(renderCat >= 0);
}

void TestAutoInspector::TestRefreshUpdatesValues() {
    InspTestObj obj;
    obj.speed = 10.0f;
    obj.active = true;
    const ClassDesc& desc = InspTestObj::Describe();
    UIContext ctx;
    int root = ctx.CreatePanel("root");
    ctx.SetRoot(root);

    InspectorResult r = GenerateInspector(&desc, &obj, ctx, root);

    // Change values externally
    obj.speed = 50.0f;
    obj.active = false;

    RefreshInspector(&desc, &obj, ctx, r.rootWidget);

    // Check slider updated
    int sld = ctx.FindWidget("insp_InspTestObj_speed_sld");
    TEST_ASSERT(sld >= 0);
    Widget* ws = ctx.GetWidget(sld);
    TEST_ASSERT_NOT_NULL(ws);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, ws->sliderValue);

    // Check checkbox updated
    int chk = ctx.FindWidget("insp_InspTestObj_active_chk");
    TEST_ASSERT(chk >= 0);
    Widget* wc = ctx.GetWidget(chk);
    TEST_ASSERT_NOT_NULL(wc);
    TEST_ASSERT_FALSE(wc->checked);
}

// -- Runner -----------------------------------------------------------

void TestAutoInspector::RunAllTests() {
    RUN_TEST(TestAutoInspector::TestNullDesc);
    RUN_TEST(TestAutoInspector::TestGenerateEmpty);
    RUN_TEST(TestAutoInspector::TestGenerateFloat);
    RUN_TEST(TestAutoInspector::TestGenerateIntSlider);
    RUN_TEST(TestAutoInspector::TestGenerateBool);
    RUN_TEST(TestAutoInspector::TestSliderBinding);
    RUN_TEST(TestAutoInspector::TestCheckboxBinding);
    RUN_TEST(TestAutoInspector::TestHiddenField);
    RUN_TEST(TestAutoInspector::TestReadOnlyField);
    RUN_TEST(TestAutoInspector::TestCategoryGrouping);
    RUN_TEST(TestAutoInspector::TestRefreshUpdatesValues);
}
