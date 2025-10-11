/**
 * @file testyawpitchroll.cpp
 * @brief Implementation of YawPitchRoll unit tests.
 */

#include "testyawpitchroll.hpp"

// ========== Constructor Tests ==========

void TestYawPitchRoll::TestDefaultConstructor() {
    YawPitchRoll ypr;

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, ypr.Yaw);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, ypr.Pitch);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, ypr.Roll);
}

void TestYawPitchRoll::TestParameterizedConstructor() {
    YawPitchRoll ypr(45.0f, 30.0f, 60.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 45.0f, ypr.Yaw);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, ypr.Pitch);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 60.0f, ypr.Roll);
}

// ========== Field Access Tests ==========

// ========== String Conversion Tests ==========

void TestYawPitchRoll::TestToString() {
    YawPitchRoll ypr(45.0f, 30.0f, 60.0f);

    ptx::UString str = ypr.ToString();
    TEST_ASSERT_TRUE(str.Length() > 0);
}

// ========== Edge Case Tests ==========

// ========== Test Runner ==========

void TestYawPitchRoll::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestYawPitchRoll::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);

    RUN_TEST(TestToString);

    RUN_TEST(TestEdgeCases);
}
