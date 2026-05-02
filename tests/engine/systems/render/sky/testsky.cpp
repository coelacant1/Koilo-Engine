// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsky.cpp
 * @brief Implementation of Sky unit tests.
 */

#include "testsky.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestSky::TestDefaultConstructor() {
    Sky obj;
    TEST_ASSERT_FALSE(obj.IsEnabled());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.0f, obj.GetTimeOfDay());
}

void TestSky::TestParameterizedConstructor() {
    // Sky only has a default constructor
    Sky obj;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, obj.GetTimeSpeed());
}

// ========== Method Tests ==========

void TestSky::TestEnable() {
    Sky obj;
    obj.Enable();
    TEST_ASSERT_TRUE(obj.IsEnabled());
}

void TestSky::TestDisable() {
    Sky obj;
    obj.Enable();
    obj.Disable();
    TEST_ASSERT_FALSE(obj.IsEnabled());
}

void TestSky::TestIsEnabled() {
    Sky obj;
    TEST_ASSERT_FALSE(obj.IsEnabled());
    obj.Enable();
    TEST_ASSERT_TRUE(obj.IsEnabled());
}

void TestSky::TestSetTimeOfDay() {
    Sky obj;
    obj.SetTimeOfDay(6.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 6.0f, obj.GetTimeOfDay());
    obj.SetTimeOfDay(25.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, obj.GetTimeOfDay());
}

void TestSky::TestGetTimeOfDay() {
    Sky obj;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.0f, obj.GetTimeOfDay());
}

void TestSky::TestSetTimeSpeed() {
    Sky obj;
    obj.SetTimeSpeed(2.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, obj.GetTimeSpeed());
}

void TestSky::TestGetTimeSpeed() {
    Sky obj;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, obj.GetTimeSpeed());
}

// ========== Edge Cases ==========

void TestSky::TestEdgeCases() {
    Sky obj;
    obj.SetTimeOfDay(-1.0f);
    TEST_ASSERT_TRUE(obj.GetTimeOfDay() >= 0.0f && obj.GetTimeOfDay() < 24.0f);
    obj.SetTimeOfDay(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, obj.GetTimeOfDay());
    Color888 top = obj.GetTopColor();
    Color888 bot = obj.GetBottomColor();
    TEST_ASSERT_TRUE(top.R <= 255 && top.G <= 255 && top.B <= 255);
    TEST_ASSERT_TRUE(bot.R <= 255 && bot.G <= 255 && bot.B <= 255);
}

// ========== Test Runner ==========

void TestSky::TestHasMaterial() {
    // TODO: Implement test for HasMaterial()
    Sky obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestSky::TestSetMaterial() {
    // TODO: Implement test for SetMaterial()
    Sky obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestSky::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEnable);
    RUN_TEST(TestDisable);
    RUN_TEST(TestIsEnabled);
    RUN_TEST(TestSetTimeOfDay);
    RUN_TEST(TestGetTimeOfDay);
    RUN_TEST(TestSetTimeSpeed);
    RUN_TEST(TestGetTimeSpeed);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestHasMaterial);
    RUN_TEST(TestSetMaterial);
}
