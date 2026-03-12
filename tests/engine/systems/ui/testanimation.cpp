// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testanimation.cpp
 * @brief Tests for TweenPool and easing functions.
 * @date 03/09/2026
 * @author Coela
 */

#include "testanimation.hpp"
#include <cmath>

using namespace koilo::ui;

// -- Helpers ---------------------------------------------------------

static float lastAppliedValue = 0.0f;
static int lastAppliedWidget = -1;
static TweenProperty lastAppliedProp = TweenProperty::PositionX;

static void TestApply(int widgetIdx, TweenProperty prop, float value, void*) {
    lastAppliedWidget = widgetIdx;
    lastAppliedProp = prop;
    lastAppliedValue = value;
}

static void ResetApply() {
    lastAppliedValue = 0.0f;
    lastAppliedWidget = -1;
    lastAppliedProp = TweenProperty::PositionX;
}

// -- Tests -----------------------------------------------------------

void TestAnimation::TestEaseLinear() {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, EaseEvaluate(EaseType::Linear, 0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, EaseEvaluate(EaseType::Linear, 0.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, EaseEvaluate(EaseType::Linear, 1.0f));
}

void TestAnimation::TestEaseInQuad() {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f,  EaseEvaluate(EaseType::EaseInQuad, 0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.25f, EaseEvaluate(EaseType::EaseInQuad, 0.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f,  EaseEvaluate(EaseType::EaseInQuad, 1.0f));
}

void TestAnimation::TestEaseOutBounce() {
    float v = EaseEvaluate(EaseType::EaseOutBounce, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, v);
    v = EaseEvaluate(EaseType::EaseOutBounce, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, v);
    // Midpoint should be > 0 and < 1
    v = EaseEvaluate(EaseType::EaseOutBounce, 0.5f);
    TEST_ASSERT_TRUE(v > 0.0f && v < 1.0f);
}

void TestAnimation::TestTweenStartCancel() {
    TweenPool pool(4);
    TEST_ASSERT_EQUAL_UINT(0, pool.ActiveCount());

    int t = pool.Start(0, TweenProperty::PositionX, 0.0f, 100.0f, 1.0f);
    TEST_ASSERT_TRUE(t >= 0);
    TEST_ASSERT_EQUAL_UINT(1, pool.ActiveCount());

    pool.Cancel(t);
    TEST_ASSERT_EQUAL_UINT(0, pool.ActiveCount());
}

void TestAnimation::TestTweenUpdate() {
    ResetApply();
    TweenPool pool(4);
    pool.Start(5, TweenProperty::Width, 10.0f, 20.0f, 1.0f);

    pool.Update(0.5f, TestApply);

    TEST_ASSERT_EQUAL_INT(5, lastAppliedWidget);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(TweenProperty::Width),
                          static_cast<int>(lastAppliedProp));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 15.0f, lastAppliedValue);
}

void TestAnimation::TestTweenDelay() {
    ResetApply();
    TweenPool pool(4);
    pool.Start(1, TweenProperty::PositionY, 0.0f, 100.0f, 1.0f,
               EaseType::Linear, 0.5f);

    // During delay - no apply
    pool.Update(0.3f, TestApply);
    TEST_ASSERT_EQUAL_INT(-1, lastAppliedWidget);

    // Past delay
    pool.Update(0.3f, TestApply);
    TEST_ASSERT_EQUAL_INT(1, lastAppliedWidget);
    TEST_ASSERT_TRUE(lastAppliedValue >= 0.0f);
}

void TestAnimation::TestTweenLoop() {
    TweenPool pool(4);
    int t = pool.Start(0, TweenProperty::PositionX, 0.0f, 10.0f, 0.5f);
    pool.SetLoop(t, true, false);

    // Complete one cycle
    int completed = pool.Update(0.6f, TestApply);
    TEST_ASSERT_EQUAL_INT(0, completed); // loop restarts, no completion
    TEST_ASSERT_EQUAL_UINT(1, pool.ActiveCount());
}

void TestAnimation::TestTweenPingPong() {
    ResetApply();
    TweenPool pool(4);
    int t = pool.Start(0, TweenProperty::PositionX, 0.0f, 10.0f, 0.5f);
    pool.SetLoop(t, false, true);

    // Forward pass completes
    pool.Update(0.6f, TestApply);
    TEST_ASSERT_EQUAL_UINT(1, pool.ActiveCount()); // still active (going back)

    // Backward pass completes
    pool.Update(0.6f, TestApply);
    TEST_ASSERT_EQUAL_UINT(0, pool.ActiveCount()); // finished full ping-pong
}

void TestAnimation::TestTweenPoolFull() {
    TweenPool pool(2);
    int t0 = pool.Start(0, TweenProperty::PositionX, 0, 1, 1);
    int t1 = pool.Start(1, TweenProperty::PositionX, 0, 1, 1);
    int t2 = pool.Start(2, TweenProperty::PositionX, 0, 1, 1);

    TEST_ASSERT_TRUE(t0 >= 0);
    TEST_ASSERT_TRUE(t1 >= 0);
    TEST_ASSERT_EQUAL_INT(-1, t2); // pool full
}

void TestAnimation::TestTweenOnComplete() {
    static int completedWidget = -1;
    completedWidget = -1;

    TweenPool pool(4);
    int t = pool.Start(7, TweenProperty::Height, 0, 50, 0.1f);
    pool.SetOnComplete(t, [](int wIdx) { completedWidget = wIdx; });

    pool.Update(0.2f, TestApply);
    TEST_ASSERT_EQUAL_INT(7, completedWidget);
}

void TestAnimation::TestCancelAll() {
    TweenPool pool(8);
    pool.Start(3, TweenProperty::PositionX, 0, 10, 1);
    pool.Start(3, TweenProperty::PositionY, 0, 20, 1);
    pool.Start(5, TweenProperty::Width, 0, 30, 1);
    TEST_ASSERT_EQUAL_UINT(3, pool.ActiveCount());

    pool.CancelAll(3);
    TEST_ASSERT_EQUAL_UINT(1, pool.ActiveCount());
}

void TestAnimation::RunAllTests() {
    RUN_TEST(TestAnimation::TestEaseLinear);
    RUN_TEST(TestAnimation::TestEaseInQuad);
    RUN_TEST(TestAnimation::TestEaseOutBounce);
    RUN_TEST(TestAnimation::TestTweenStartCancel);
    RUN_TEST(TestAnimation::TestTweenUpdate);
    RUN_TEST(TestAnimation::TestTweenDelay);
    RUN_TEST(TestAnimation::TestTweenLoop);
    RUN_TEST(TestAnimation::TestTweenPingPong);
    RUN_TEST(TestAnimation::TestTweenPoolFull);
    RUN_TEST(TestAnimation::TestTweenOnComplete);
    RUN_TEST(TestAnimation::TestCancelAll);
}
