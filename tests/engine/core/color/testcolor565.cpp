// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcolor565.cpp
 * @brief Implementation of Color565 unit tests.
 */

#include "testcolor565.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestColor565::TestDefaultConstructor() {
    Color565 color;
    TEST_ASSERT_EQUAL_UINT16(0, color.GetPacked());
    TEST_ASSERT_EQUAL_UINT8(0, color.GetR5());
    TEST_ASSERT_EQUAL_UINT8(0, color.GetG6());
    TEST_ASSERT_EQUAL_UINT8(0, color.GetB5());
}

void TestColor565::TestParameterizedConstructor() {
    Color565 c1(0xF800);  // Red in RGB565
    TEST_ASSERT_EQUAL_UINT16(0xF800, c1.GetPacked());
    
    Color565 c2(255, 0, 0);  // Red from RGB888
    TEST_ASSERT_EQUAL_UINT8(31, c2.GetR5());  // Max red in 5-bit
    TEST_ASSERT_EQUAL_UINT8(0, c2.GetG6());
    TEST_ASSERT_EQUAL_UINT8(0, c2.GetB5());
}

// ========== Method Tests ==========

void TestColor565::TestSet() {
    Color565 color;
    color.Set(0x07E0);  // Green in RGB565
    TEST_ASSERT_EQUAL_UINT16(0x07E0, color.GetPacked());
    
    color.Set(0, 255, 0);  // Green from RGB888
    TEST_ASSERT_EQUAL_UINT8(0, color.GetR5());
    TEST_ASSERT_EQUAL_UINT8(63, color.GetG6());  // Max green in 6-bit
    TEST_ASSERT_EQUAL_UINT8(0, color.GetB5());
}

void TestColor565::TestGetPacked() {
    Color565 color(0x001F);  // Blue in RGB565
    TEST_ASSERT_EQUAL_UINT16(0x001F, color.GetPacked());
}

void TestColor565::TestGetR5() {
    Color565 red(255, 0, 0);
    TEST_ASSERT_EQUAL_UINT8(31, red.GetR5());  // 5-bit max
}

void TestColor565::TestGetG6() {
    Color565 green(0, 255, 0);
    TEST_ASSERT_EQUAL_UINT8(63, green.GetG6());  // 6-bit max
}

void TestColor565::TestGetB5() {
    Color565 blue(0, 0, 255);
    TEST_ASSERT_EQUAL_UINT8(31, blue.GetB5());  // 5-bit max
}

void TestColor565::TestGetR8() {
    Color565 red(255, 0, 0);
    uint8_t r8 = red.GetR8();
    TEST_ASSERT_UINT8_WITHIN(10, 248, r8);  // 31 << 3 = 248
}

void TestColor565::TestGetG8() {
    Color565 green(0, 255, 0);
    uint8_t g8 = green.GetG8();
    TEST_ASSERT_UINT8_WITHIN(10, 252, g8);  // 63 << 2 = 252
}

void TestColor565::TestGetB8() {
    Color565 blue(0, 0, 255);
    uint8_t b8 = blue.GetB8();
    TEST_ASSERT_UINT8_WITHIN(10, 248, b8);  // 31 << 3 = 248
}

void TestColor565::TestToString() {
    Color565 color(0xF800);
    koilo::UString str = color.ToString();
    TEST_ASSERT_TRUE(str.Length() > 0);
}

// ========== Edge Cases ==========

void TestColor565::TestEdgeCases() {
    // Test black
    Color565 black(0, 0, 0);
    TEST_ASSERT_EQUAL_UINT16(0x0000, black.GetPacked());
    
    // Test white
    Color565 white(255, 255, 255);
    TEST_ASSERT_EQUAL_UINT8(31, white.GetR5());
    TEST_ASSERT_EQUAL_UINT8(63, white.GetG6());
    TEST_ASSERT_EQUAL_UINT8(31, white.GetB5());
    
    // Test packed white
    Color565 whiteP(0xFFFF);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, whiteP.GetPacked());
}

// ========== Test Runner ==========

void TestColor565::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSet);
    RUN_TEST(TestGetPacked);
    RUN_TEST(TestGetR5);
    RUN_TEST(TestGetG6);
    RUN_TEST(TestGetB5);
    RUN_TEST(TestGetR8);
    RUN_TEST(TestGetG8);
    RUN_TEST(TestGetB8);
    RUN_TEST(TestToString);
    RUN_TEST(TestEdgeCases);
}
