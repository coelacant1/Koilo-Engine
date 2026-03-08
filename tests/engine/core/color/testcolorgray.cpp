// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcolorgray.cpp
 * @brief Implementation of ColorGray unit tests.
 */

#include "testcolorgray.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestColorGray::TestDefaultConstructor() {
    // Test default constructor initializes to black (0)
    ColorGray color;
    TEST_ASSERT_EQUAL_UINT8(0, color.Get());
    TEST_ASSERT_EQUAL_UINT8(0, color.value);
}

void TestColorGray::TestParameterizedConstructor() {
    // Test grayscale value constructor
    ColorGray black(0);
    TEST_ASSERT_EQUAL_UINT8(0, black.Get());
    
    ColorGray midGray(128);
    TEST_ASSERT_EQUAL_UINT8(128, midGray.Get());
    
    ColorGray white(255);
    TEST_ASSERT_EQUAL_UINT8(255, white.Get());
    
    // Test RGB constructor
    ColorGray fromRed(255, 0, 0);
    // Red's grayscale value: 0.299 * 255 = ~76
    TEST_ASSERT_UINT8_WITHIN(5, 76, fromRed.Get());
    
    ColorGray fromGreen(0, 255, 0);
    // Green's grayscale value: 0.587 * 255 = ~150
    TEST_ASSERT_UINT8_WITHIN(5, 150, fromGreen.Get());
    
    ColorGray fromBlue(0, 0, 255);
    // Blue's grayscale value: 0.114 * 255 = ~29
    TEST_ASSERT_UINT8_WITHIN(5, 29, fromBlue.Get());
    
    ColorGray fromWhite(255, 255, 255);
    TEST_ASSERT_EQUAL_UINT8(255, fromWhite.Get());
}

// ========== Method Tests ==========

void TestColorGray::TestSet() {
    ColorGray color;
    
    color.Set(64);
    TEST_ASSERT_EQUAL_UINT8(64, color.Get());
    
    color.Set(200);
    TEST_ASSERT_EQUAL_UINT8(200, color.Get());
    
    color.Set(0);
    TEST_ASSERT_EQUAL_UINT8(0, color.Get());
    
    color.Set(255);
    TEST_ASSERT_EQUAL_UINT8(255, color.Get());
}

void TestColorGray::TestGet() {
    ColorGray gray128(128);
    TEST_ASSERT_EQUAL_UINT8(128, gray128.Get());
    
    ColorGray gray255(255);
    TEST_ASSERT_EQUAL_UINT8(255, gray255.Get());
}

void TestColorGray::TestScale() {
    ColorGray color(200);
    
    // Scale with maxBrightness 128 (50%)
    ColorGray scaled = color.Scale(128);
    TEST_ASSERT_UINT8_WITHIN(2, 100, scaled.Get());
    
    // Scale with maxBrightness 255 (100%)
    ColorGray fullBright = color.Scale(255);
    TEST_ASSERT_EQUAL_UINT8(200, fullBright.Get());
    
    // Scale with maxBrightness 0
    ColorGray dark = color.Scale(0);
    TEST_ASSERT_EQUAL_UINT8(0, dark.Get());
}

void TestColorGray::TestAdd() {
    ColorGray color(100);
    
    // Add 50 to 100 = 150
    ColorGray added = color.Add(50);
    TEST_ASSERT_EQUAL_UINT8(150, added.Get());
    
    // Add to black
    ColorGray black(0);
    ColorGray addedToBlack = black.Add(50);
    TEST_ASSERT_EQUAL_UINT8(50, addedToBlack.Get());
    
    // Test overflow - add exceeding 255 should cap at 255
    ColorGray bright(250);
    ColorGray overflow = bright.Add(100);
    TEST_ASSERT_EQUAL_UINT8(255, overflow.Get());
}

void TestColorGray::TestToString() {
    ColorGray gray128(128);
    koilo::UString str128 = gray128.ToString();
    TEST_ASSERT_TRUE(str128.Length() > 0);
    
    ColorGray gray255(255);
    koilo::UString str255 = gray255.ToString();
    TEST_ASSERT_TRUE(str255.Length() > 0);
    
    ColorGray gray0(0);
    koilo::UString str0 = gray0.ToString();
    TEST_ASSERT_TRUE(str0.Length() > 0);
}

// ========== Edge Cases ==========

void TestColorGray::TestEdgeCases() {
    // Test min/max bounds
    ColorGray minVal(0);
    TEST_ASSERT_EQUAL_UINT8(0, minVal.Get());
    
    ColorGray maxVal(255);
    TEST_ASSERT_EQUAL_UINT8(255, maxVal.Get());
    
    // Test operators
    ColorGray g1(100);
    ColorGray g2(100);
    ColorGray g3(200);
    
    // Equality operators
    TEST_ASSERT_TRUE(g1 == g2);
    TEST_ASSERT_FALSE(g1 == g3);
    TEST_ASSERT_FALSE(g1 != g2);
    TEST_ASSERT_TRUE(g1 != g3);
    
    // Comparison operators
    TEST_ASSERT_TRUE(g1 < g3);
    TEST_ASSERT_FALSE(g3 < g1);
    TEST_ASSERT_FALSE(g1 < g2);
    TEST_ASSERT_TRUE(g3 > g1);
    TEST_ASSERT_FALSE(g1 > g3);
    
    // Test addition operator
    ColorGray sum = g1 + g2;
    TEST_ASSERT_EQUAL_UINT8(200, sum.Get());
    
    // Test addition with overflow
    ColorGray highVal(240);
    ColorGray highSum = highVal + highVal;
    TEST_ASSERT_EQUAL_UINT8(255, highSum.Get());
    
    // Test subtraction operator
    ColorGray diff = g3 - g1;
    TEST_ASSERT_EQUAL_UINT8(100, diff.Get());
    
    // Test subtraction with underflow
    ColorGray lowVal(50);
    ColorGray lowDiff = lowVal - g3;
    TEST_ASSERT_EQUAL_UINT8(0, lowDiff.Get());
    
    // Test scalar multiplication
    ColorGray scaled = g1 * 0.5f;
    TEST_ASSERT_UINT8_WITHIN(2, 50, scaled.Get());
    
    ColorGray scaled2 = g1 * 2.0f;
    TEST_ASSERT_EQUAL_UINT8(200, scaled2.Get());
    
    // Test ToMonochrome conversion (threshold at 128)
    ColorGray dark(100);
    TEST_ASSERT_FALSE(dark.ToMonochrome());
    
    ColorGray bright(200);
    TEST_ASSERT_TRUE(bright.ToMonochrome());
    
    // Test IColor interface
    uint8_t r, g, b;
    g1.ToRGB888(r, g, b);
    TEST_ASSERT_EQUAL_UINT8(100, r);
    TEST_ASSERT_EQUAL_UINT8(100, g);
    TEST_ASSERT_EQUAL_UINT8(100, b);
}

// ========== Test Runner ==========

void TestColorGray::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSet);
    RUN_TEST(TestGet);
    RUN_TEST(TestScale);
    RUN_TEST(TestAdd);
    RUN_TEST(TestToString);
    RUN_TEST(TestEdgeCases);
}
