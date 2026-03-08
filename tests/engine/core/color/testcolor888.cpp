// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcolor888.cpp
 * @brief Implementation of Color888 unit tests.
 */

#include "testcolor888.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestColor888::TestDefaultConstructor() {
    Color888 color;
    TEST_ASSERT_EQUAL_UINT8(0, color.r);
    TEST_ASSERT_EQUAL_UINT8(0, color.g);
    TEST_ASSERT_EQUAL_UINT8(0, color.b);
}

void TestColor888::TestParameterizedConstructor() {
    Color888 color(255, 128, 64);
    TEST_ASSERT_EQUAL_UINT8(255, color.r);
    TEST_ASSERT_EQUAL_UINT8(128, color.g);
    TEST_ASSERT_EQUAL_UINT8(64, color.b);
}

// ========== Method Tests ==========

void TestColor888::TestSet() {
    Color888 color;
    color.Set(100, 150, 200);
    TEST_ASSERT_EQUAL_UINT8(100, color.r);
    TEST_ASSERT_EQUAL_UINT8(150, color.g);
    TEST_ASSERT_EQUAL_UINT8(200, color.b);
}

void TestColor888::TestScale() {
    Color888 color(200, 100, 50);
    Color888 scaled = color.Scale(128);
    TEST_ASSERT_UINT8_WITHIN(2, 100, scaled.r);
    TEST_ASSERT_UINT8_WITHIN(2, 50, scaled.g);
    TEST_ASSERT_UINT8_WITHIN(2, 25, scaled.b);
}

void TestColor888::TestAdd() {
    Color888 color(100, 50, 25);
    Color888 added = color.Add(50);
    TEST_ASSERT_EQUAL_UINT8(150, added.r);
    TEST_ASSERT_EQUAL_UINT8(100, added.g);
    TEST_ASSERT_EQUAL_UINT8(75, added.b);
}

void TestColor888::TestHueShift() {
    Color888 red(255, 0, 0);
    Color888 shifted = red.HueShift(120.0f);
    TEST_ASSERT_UINT8_WITHIN(10, 0, shifted.r);
    TEST_ASSERT_UINT8_WITHIN(10, 255, shifted.g);
    TEST_ASSERT_UINT8_WITHIN(10, 0, shifted.b);
}

void TestColor888::TestToString() {
    Color888 color(100, 150, 200);
    koilo::UString str = color.ToString();
    TEST_ASSERT_TRUE(str.Length() > 0);
}

// ========== Edge Cases ==========

void TestColor888::TestEdgeCases() {
    Color888 black(0, 0, 0);
    TEST_ASSERT_EQUAL_UINT8(0, black.r);
    
    Color888 white(255, 255, 255);
    TEST_ASSERT_EQUAL_UINT8(255, white.r);
    
    Color888 overflow(250, 250, 250);
    Color888 added = overflow.Add(100);
    TEST_ASSERT_EQUAL_UINT8(255, added.r);
}

// ========== Test Runner ==========

void TestColor888::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSet);
    RUN_TEST(TestScale);
    RUN_TEST(TestAdd);
    RUN_TEST(TestHueShift);
    RUN_TEST(TestToString);
    RUN_TEST(TestEdgeCases);
}
