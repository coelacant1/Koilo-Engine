// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcolormono.cpp
 * @brief Implementation of ColorMono unit tests.
 */

#include "testcolormono.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestColorMono::TestDefaultConstructor() {
    // Test default constructor initializes to off (black)
    ColorMono color;
    TEST_ASSERT_FALSE(color.Get());
    TEST_ASSERT_EQUAL_UINT8(0, color.ToGrayscale8());
}

void TestColorMono::TestParameterizedConstructor() {
    // Test boolean constructor - ON
    ColorMono on(true);
    TEST_ASSERT_TRUE(on.Get());
    TEST_ASSERT_EQUAL_UINT8(255, on.ToGrayscale8());
    
    // Test boolean constructor - OFF
    ColorMono off(false);
    TEST_ASSERT_FALSE(off.Get());
    TEST_ASSERT_EQUAL_UINT8(0, off.ToGrayscale8());
    
    // Test grayscale constructor with default threshold (128)
    ColorMono darkGray(uint8_t(64));  // Below threshold -> OFF
    TEST_ASSERT_FALSE(darkGray.Get());
    
    ColorMono brightGray(uint8_t(200));  // Above threshold -> ON
    TEST_ASSERT_TRUE(brightGray.Get());
    
    // Test grayscale constructor with custom threshold
    ColorMono custom(uint8_t(127), uint8_t(127));  // At threshold -> ON (>=)
    TEST_ASSERT_TRUE(custom.Get());
    
    ColorMono custom2(uint8_t(128), uint8_t(128));  // Exactly at threshold -> ON
    TEST_ASSERT_TRUE(custom2.Get());
    
    // Test RGB constructor
    ColorMono rgb(255, 0, 0);  // Red - grayscale ≈ 76 (below default 128 threshold) -> OFF
    TEST_ASSERT_FALSE(rgb.Get());
    
    ColorMono darkRgb(20, 20, 20);  // Dark -> OFF
    TEST_ASSERT_FALSE(darkRgb.Get());
}

// ========== Method Tests ==========

void TestColorMono::TestSet() {
    ColorMono color;
    
    // Test Set with true
    color.Set(true);
    TEST_ASSERT_TRUE(color.Get());
    
    // Test Set with false
    color.Set(false);
    TEST_ASSERT_FALSE(color.Get());
}

void TestColorMono::TestGet() {
    ColorMono on(true);
    TEST_ASSERT_TRUE(on.Get());
    
    ColorMono off(false);
    TEST_ASSERT_FALSE(off.Get());
}

void TestColorMono::TestTurnOn() {
    ColorMono color(false);
    TEST_ASSERT_FALSE(color.Get());
    
    color.TurnOn();
    TEST_ASSERT_TRUE(color.Get());
    
    // Multiple TurnOn calls should remain ON
    color.TurnOn();
    TEST_ASSERT_TRUE(color.Get());
}

void TestColorMono::TestTurnOff() {
    ColorMono color(true);
    TEST_ASSERT_TRUE(color.Get());
    
    color.TurnOff();
    TEST_ASSERT_FALSE(color.Get());
    
    // Multiple TurnOff calls should remain OFF
    color.TurnOff();
    TEST_ASSERT_FALSE(color.Get());
}

void TestColorMono::TestToggle() {
    ColorMono color(false);
    
    // First toggle - OFF to ON
    color.Toggle();
    TEST_ASSERT_TRUE(color.Get());
    
    // Second toggle - ON to OFF
    color.Toggle();
    TEST_ASSERT_FALSE(color.Get());
    
    // Third toggle - OFF to ON
    color.Toggle();
    TEST_ASSERT_TRUE(color.Get());
}

void TestColorMono::TestToString() {
    ColorMono on(true);
    koilo::UString onStr = on.ToString();
    TEST_ASSERT_TRUE(onStr.Length() > 0);
    
    ColorMono off(false);
    koilo::UString offStr = off.ToString();
    TEST_ASSERT_TRUE(offStr.Length() > 0);
}

// ========== Edge Cases ==========

void TestColorMono::TestEdgeCases() {
    // Test IColor interface conversions
    ColorMono on(true);
    uint8_t r, g, b;
    on.ToRGB888(r, g, b);
    TEST_ASSERT_EQUAL_UINT8(255, r);
    TEST_ASSERT_EQUAL_UINT8(255, g);
    TEST_ASSERT_EQUAL_UINT8(255, b);
    
    ColorMono off(false);
    off.ToRGB888(r, g, b);
    TEST_ASSERT_EQUAL_UINT8(0, r);
    TEST_ASSERT_EQUAL_UINT8(0, g);
    TEST_ASSERT_EQUAL_UINT8(0, b);
    
    // Test RGB565 conversion
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, on.ToRGB565());
    TEST_ASSERT_EQUAL_UINT16(0x0000, off.ToRGB565());
    
    // Test monochrome conversion
    TEST_ASSERT_TRUE(on.ToMonochrome());
    TEST_ASSERT_FALSE(off.ToMonochrome());
    
    // Test operators
    ColorMono aa(true);
    ColorMono bb(true);
    ColorMono cc(false);
    
    TEST_ASSERT_TRUE(aa == bb);
    TEST_ASSERT_FALSE(aa == cc);
    TEST_ASSERT_FALSE(aa != bb);
    TEST_ASSERT_TRUE(aa != cc);
    
    // Test logical operators
    ColorMono andResult = aa && cc;
    TEST_ASSERT_FALSE(andResult.Get());
    
    ColorMono orResult = aa || cc;
    TEST_ASSERT_TRUE(orResult.Get());
    
    ColorMono xorResult = aa ^ cc;
    TEST_ASSERT_TRUE(xorResult.Get());
    
    ColorMono notResult = !aa;
    TEST_ASSERT_FALSE(notResult.Get());
}

// ========== Test Runner ==========

void TestColorMono::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSet);
    RUN_TEST(TestGet);
    RUN_TEST(TestTurnOn);
    RUN_TEST(TestTurnOff);
    RUN_TEST(TestToggle);
    RUN_TEST(TestToString);
    RUN_TEST(TestEdgeCases);
}
