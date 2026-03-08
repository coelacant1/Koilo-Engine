// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testyawpitchroll.cpp
 * @brief Implementation of YawPitchRoll unit tests.
 */

#include "testyawpitchroll.hpp"

using namespace koilo;
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

    koilo::UString str = ypr.ToString();
    TEST_ASSERT_TRUE(str.Length() > 0);
}

// ========== Edge Case Tests ==========

// ========== Test Runner ==========

void TestYawPitchRoll::TestEdgeCases() {
    YawPitchRoll zero;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, zero.Yaw);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, zero.Pitch);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, zero.Roll);
    
    YawPitchRoll full(360.0f, 90.0f, 180.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 360.0f, full.Yaw);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, full.Pitch);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 180.0f, full.Roll);
    
    YawPitchRoll negative(-45.0f, -30.0f, -60.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -45.0f, negative.Yaw);
}

void TestYawPitchRoll::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);

    RUN_TEST(TestToString);

    RUN_TEST(TestEdgeCases);
}
