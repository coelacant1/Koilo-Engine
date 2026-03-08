// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testdirectionangle.cpp
 * @brief Implementation of DirectionAngle unit tests.
 */

#include "testdirectionangle.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

// ========== Field Access Tests ==========

// ========== String Conversion Tests ==========

void TestDirectionAngle::TestToString() {
    DirectionAngle da(90.0f, Vector3D(1.0f, 0.0f, 0.0f));

    koilo::UString str = da.ToString();
    TEST_ASSERT_TRUE(str.Length() > 0);
}

// ========== Direction Vector Tests ==========

// ========== Edge Case Tests ==========

// ========== Test Runner ==========

void TestDirectionAngle::TestDefaultConstructor() {
    DirectionAngle da(0.0f, 1.0f, 0.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, da.Rotation);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, da.Direction.X);
}

void TestDirectionAngle::TestEdgeCases() {
    DirectionAngle zero(0.0f, Vector3D(1.0f, 0.0f, 0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, zero.Rotation);
    
    DirectionAngle full(360.0f, Vector3D(0.0f, 0.0f, 1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 360.0f, full.Rotation);
    
    DirectionAngle negative(-90.0f, Vector3D(1.0f, 1.0f, 1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -90.0f, negative.Rotation);
}

void TestDirectionAngle::TestParameterizedConstructor() {
    DirectionAngle da1(45.0f, 1.0f, 0.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 45.0f, da1.Rotation);
    
    Vector3D dir(0.0f, 1.0f, 0.0f);
    DirectionAngle da2(90.0f, dir);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, da2.Rotation);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, da2.Direction.Y);
}

void TestDirectionAngle::RunAllTests() {

    RUN_TEST(TestToString);

    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestParameterizedConstructor);
}
