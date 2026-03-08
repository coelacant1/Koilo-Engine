// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testgradientcolor.cpp
 * @brief Implementation of GradientColor unit tests.
 */

#include "testgradientcolor.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestGradientColor::TestDefaultConstructor() {
    GradientColor gradient;
    TEST_ASSERT_EQUAL(0, gradient.GetColorCount());
    TEST_ASSERT_FALSE(gradient.IsStepped());
}

void TestGradientColor::TestParameterizedConstructor() {
    // Test constructor with vector
    std::vector<Color888> colors = {
        Color888(0, 0, 0),        // Black
        Color888(128, 128, 128),  // Gray
        Color888(255, 255, 255)   // White
    };
    
    GradientColor gradientSmooth(colors, false);
    TEST_ASSERT_EQUAL(3, gradientSmooth.GetColorCount());
    TEST_ASSERT_FALSE(gradientSmooth.IsStepped());
    
    GradientColor gradientStepped(colors, true);
    TEST_ASSERT_EQUAL(3, gradientStepped.GetColorCount());
    TEST_ASSERT_TRUE(gradientStepped.IsStepped());
    
    // Test constructor with pointer and count
    Color888 colorArray[] = {
        Color888(255, 0, 0),    // Red
        Color888(0, 255, 0),    // Green
        Color888(0, 0, 255)     // Blue
    };
    
    GradientColor gradientFromArray(colorArray, 3, false);
    TEST_ASSERT_EQUAL(3, gradientFromArray.GetColorCount());
    TEST_ASSERT_FALSE(gradientFromArray.IsStepped());
    
    // Test constructor with nullptr and count 0
    GradientColor emptyGradient(nullptr, 0, false);
    TEST_ASSERT_EQUAL(0, emptyGradient.GetColorCount());
}

// ========== Method Tests ==========

void TestGradientColor::TestGetColorAt() {
    std::vector<Color888> colors = {
        Color888(0, 0, 0),       // Black
        Color888(255, 255, 255)  // White
    };
    
    GradientColor gradient(colors, false);
    
    // Test start of gradient (ratio = 0.0)
    Color888 startColor = gradient.GetColorAt(0.0f);
    TEST_ASSERT_EQUAL_UINT8(0, startColor.r);
    TEST_ASSERT_EQUAL_UINT8(0, startColor.g);
    TEST_ASSERT_EQUAL_UINT8(0, startColor.b);
    
    // Test end of gradient (ratio = 1.0)
    Color888 endColor = gradient.GetColorAt(1.0f);
    TEST_ASSERT_EQUAL_UINT8(255, endColor.r);
    TEST_ASSERT_EQUAL_UINT8(255, endColor.g);
    TEST_ASSERT_EQUAL_UINT8(255, endColor.b);
    
    // Test midpoint (ratio = 0.5) - should interpolate
    Color888 midColor = gradient.GetColorAt(0.5f);
    TEST_ASSERT_UINT8_WITHIN(2, 127, midColor.r);
    TEST_ASSERT_UINT8_WITHIN(2, 127, midColor.g);
    TEST_ASSERT_UINT8_WITHIN(2, 127, midColor.b);
    
    // Test clamping - ratio < 0.0 should clamp to start
    Color888 clampedStart = gradient.GetColorAt(-0.5f);
    TEST_ASSERT_EQUAL_UINT8(0, clampedStart.r);
    
    // Test clamping - ratio > 1.0 should clamp to end
    Color888 clampedEnd = gradient.GetColorAt(1.5f);
    TEST_ASSERT_EQUAL_UINT8(255, clampedEnd.r);
}

void TestGradientColor::TestSetColors() {
    GradientColor gradient;
    
    // Initially empty
    TEST_ASSERT_EQUAL(0, gradient.GetColorCount());
    
    // Set colors from vector
    std::vector<Color888> colors = {
        Color888(255, 0, 0),    // Red
        Color888(0, 255, 0),    // Green
        Color888(0, 0, 255)     // Blue
    };
    
    gradient.SetColors(colors);
    TEST_ASSERT_EQUAL(3, gradient.GetColorCount());
    
    // Set colors from array
    Color888 newColors[] = {
        Color888(100, 100, 100),
        Color888(200, 200, 200)
    };
    
    gradient.SetColors(newColors, 2);
    TEST_ASSERT_EQUAL(2, gradient.GetColorCount());
    
    // Clear colors with nullptr
    gradient.SetColors(nullptr, 0);
    TEST_ASSERT_EQUAL(0, gradient.GetColorCount());
}

void TestGradientColor::TestGetColorCount() {
    std::vector<Color888> colors = {Color888(0, 0, 0), Color888(128, 128, 128), Color888(255, 255, 255)};
    GradientColor gradient(colors, false);
    TEST_ASSERT_EQUAL(3, gradient.GetColorCount());
}

void TestGradientColor::TestIsStepped() {
    GradientColor gradientSmooth;
    GradientColor gradientStepped;

    std::vector<Color888> colors = {Color888(0, 0, 0), Color888(255, 255, 255)};
    gradientSmooth = GradientColor(colors, false);
    gradientStepped = GradientColor(colors, true);

    TEST_ASSERT_FALSE(gradientSmooth.IsStepped());
    TEST_ASSERT_TRUE(gradientStepped.IsStepped());
}

void TestGradientColor::TestSetStepped() {
    std::vector<Color888> colors = {Color888(0, 0, 0), Color888(255, 255, 255)};
    GradientColor gradient(colors, false);

    TEST_ASSERT_FALSE(gradient.IsStepped());

    gradient.SetStepped(true);
    TEST_ASSERT_TRUE(gradient.IsStepped());

    gradient.SetStepped(false);
    TEST_ASSERT_FALSE(gradient.IsStepped());
}

// ========== Edge Cases ==========

void TestGradientColor::TestEdgeCases() {
    // Test empty gradient
    GradientColor emptyGradient;
    Color888 emptyColor = emptyGradient.GetColorAt(0.5f);
    TEST_ASSERT_EQUAL_UINT8(0, emptyColor.r);
    TEST_ASSERT_EQUAL_UINT8(0, emptyColor.g);
    TEST_ASSERT_EQUAL_UINT8(0, emptyColor.b);
    
    // Test single color gradient
    std::vector<Color888> singleColor = {Color888(100, 150, 200)};
    GradientColor singleGradient(singleColor, false);
    
    Color888 color1 = singleGradient.GetColorAt(0.0f);
    TEST_ASSERT_EQUAL_UINT8(100, color1.r);
    
    Color888 color2 = singleGradient.GetColorAt(0.5f);
    TEST_ASSERT_EQUAL_UINT8(100, color2.r);
    
    Color888 color3 = singleGradient.GetColorAt(1.0f);
    TEST_ASSERT_EQUAL_UINT8(100, color3.r);
    
    // Test stepped interpolation
    std::vector<Color888> steppedColors = {
        Color888(0, 0, 0),      // Black
        Color888(255, 0, 0)     // Red
    };
    
    GradientColor steppedGradient(steppedColors, true);
    
    // In stepped mode, ratio < 0.5 should return first color
    Color888 step1 = steppedGradient.GetColorAt(0.25f);
    TEST_ASSERT_EQUAL_UINT8(0, step1.r);
    
    // In stepped mode with 2 colors, ratio < 1.0 still returns first color
    Color888 step2 = steppedGradient.GetColorAt(0.75f);
    TEST_ASSERT_EQUAL_UINT8(0, step2.r);
    
    // Test smooth interpolation vs stepped
    GradientColor smoothGradient(steppedColors, false);
    
    Color888 smooth = smoothGradient.GetColorAt(0.5f);
    // At 0.5, should be roughly halfway between 0 and 255
    TEST_ASSERT_UINT8_WITHIN(2, 127, smooth.r);
    
    // Test three-color gradient for interpolation
    std::vector<Color888> threeColors = {
        Color888(0, 0, 0),      // Black
        Color888(128, 128, 128),// Gray
        Color888(255, 255, 255) // White
    };
    
    GradientColor threeColorGradient(threeColors, false);
    
    // At 0.25, should be between black and gray
    Color888 quarter = threeColorGradient.GetColorAt(0.25f);
    TEST_ASSERT_UINT8_WITHIN(10, 64, quarter.r);
    
    // At 0.75, should be between gray and white
    Color888 threeQuarters = threeColorGradient.GetColorAt(0.75f);
    TEST_ASSERT_UINT8_WITHIN(10, 192, threeQuarters.r);
}

// ========== Test Runner ==========

void TestGradientColor::TestSetColorsFromPalette() {
    // TODO: Implement test for SetColorsFromPalette()
    GradientColor obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestGradientColor::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetColorAt);
    RUN_TEST(TestSetColors);
    RUN_TEST(TestGetColorCount);
    RUN_TEST(TestIsStepped);
    RUN_TEST(TestSetStepped);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestSetColorsFromPalette);
}
