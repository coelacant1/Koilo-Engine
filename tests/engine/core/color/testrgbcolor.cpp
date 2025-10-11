/**
 * @file testrgbcolor.cpp
 * @brief Implementation of RGBColor unit tests.
 */

#include "testrgbcolor.hpp"

// ========== Constructor Tests ==========

void TestRGBColor::TestDefaultConstructor() {
    RGBColor color;
    TEST_ASSERT_EQUAL_UINT8(0, color.R);
    TEST_ASSERT_EQUAL_UINT8(0, color.G);
    TEST_ASSERT_EQUAL_UINT8(0, color.B);
}

void TestRGBColor::TestParameterizedConstructor() {
    RGBColor color(255, 128, 64);
    TEST_ASSERT_EQUAL_UINT8(255, color.R);
    TEST_ASSERT_EQUAL_UINT8(128, color.G);
    TEST_ASSERT_EQUAL_UINT8(64, color.B);
}

// ========== Setter Tests ==========

void TestRGBColor::TestSetColor() {
    RGBColor color;
    color.SetColor(50, 100, 150);
    TEST_ASSERT_EQUAL_UINT8(50, color.R);
    TEST_ASSERT_EQUAL_UINT8(100, color.G);
    TEST_ASSERT_EQUAL_UINT8(150, color.B);
}

// ========== Color Manipulation Tests ==========

void TestRGBColor::TestScale() {
    RGBColor color(200, 100, 50);
    RGBColor scaled = color.Scale(128);
    // Expect proportional scaling based on max component
    // Max component is 200, scaling to 128 -> ratio = 128/200 = 0.64
    TEST_ASSERT_UINT8_WITHIN(2, 128, scaled.R);  // 200 * 0.64 = 128
    TEST_ASSERT_UINT8_WITHIN(2, 64, scaled.G);   // 100 * 0.64 = 64
    TEST_ASSERT_UINT8_WITHIN(2, 32, scaled.B);   // 50 * 0.64 = 32
}

void TestRGBColor::TestAdd() {
    RGBColor color(100, 50, 25);
    RGBColor added = color.Add(50);
    TEST_ASSERT_EQUAL_UINT8(150, added.R);
    TEST_ASSERT_EQUAL_UINT8(100, added.G);
    TEST_ASSERT_EQUAL_UINT8(75, added.B);
}

void TestRGBColor::TestHueShift() {
    // Pure red shifted by 120° should become green
    RGBColor red(255, 0, 0);
    RGBColor shifted = red.HueShift(120.0f);
    // Hue shift converts to HSV, shifts hue, converts back
    // Pure red (H=0) + 120° = H=120° (green)
    TEST_ASSERT_UINT8_WITHIN(5, 0, shifted.R);
    TEST_ASSERT_UINT8_WITHIN(5, 255, shifted.G);
    TEST_ASSERT_UINT8_WITHIN(5, 0, shifted.B);
}

// ========== Static Method Tests ==========

// ========== Operator Tests ==========

// ========== Utility Tests ==========

void TestRGBColor::TestToString() {
    RGBColor color(100, 150, 200);
    ptx::UString str = color.ToString();
    // Expected format: "(100, 150, 200)" or similar
    TEST_ASSERT_TRUE(str.Length() > 0);
}

// ========== Test Runner ==========

void TestRGBColor::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestRGBColor::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);

    RUN_TEST(TestSetColor);

    RUN_TEST(TestScale);

    RUN_TEST(TestAdd);

    RUN_TEST(TestHueShift);

    RUN_TEST(TestToString);
    RUN_TEST(TestEdgeCases);
}
