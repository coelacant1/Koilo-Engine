/**
 * @file testgradientcolor.cpp
 * @brief Implementation of GradientColor unit tests.
 */

#include "testgradientcolor.hpp"

// ========== Constructor Tests ==========

void TestGradientColor::TestDefaultConstructor() {
    GradientColor gradient;
    TEST_ASSERT_EQUAL(0, gradient.GetColorCount());
    TEST_ASSERT_FALSE(gradient.IsStepped());
}

void TestGradientColor::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_ASSERT_TRUE(false);  // Not implemented
}

// ========== Method Tests ==========

void TestGradientColor::TestGetColorAt() {
    // TODO: Implement test for GetColorAt()
    // GradientColor obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestGradientColor::TestSetColors() {
    // TODO: Implement test for SetColors()
    // GradientColor obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestGradientColor::TestGetColorCount() {
    std::vector<RGBColor> colors = {RGBColor(0, 0, 0), RGBColor(128, 128, 128), RGBColor(255, 255, 255)};
    GradientColor gradient(colors, false);
    TEST_ASSERT_EQUAL(3, gradient.GetColorCount());
}

void TestGradientColor::TestIsStepped() {
    GradientColor gradientSmooth;
    GradientColor gradientStepped;

    std::vector<RGBColor> colors = {RGBColor(0, 0, 0), RGBColor(255, 255, 255)};
    gradientSmooth = GradientColor(colors, false);
    gradientStepped = GradientColor(colors, true);

    TEST_ASSERT_FALSE(gradientSmooth.IsStepped());
    TEST_ASSERT_TRUE(gradientStepped.IsStepped());
}

void TestGradientColor::TestSetStepped() {
    std::vector<RGBColor> colors = {RGBColor(0, 0, 0), RGBColor(255, 255, 255)};
    GradientColor gradient(colors, false);

    TEST_ASSERT_FALSE(gradient.IsStepped());

    gradient.SetStepped(true);
    TEST_ASSERT_TRUE(gradient.IsStepped());

    gradient.SetStepped(false);
    TEST_ASSERT_FALSE(gradient.IsStepped());
}

// ========== Edge Cases ==========

void TestGradientColor::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_ASSERT_TRUE(false);  // Not implemented
}

// ========== Test Runner ==========

void TestGradientColor::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetColorAt);
    RUN_TEST(TestSetColors);
    RUN_TEST(TestGetColorCount);
    RUN_TEST(TestIsStepped);
    RUN_TEST(TestSetStepped);
    RUN_TEST(TestEdgeCases);
}
