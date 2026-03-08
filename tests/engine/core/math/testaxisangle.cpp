// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testaxisangle.cpp
 * @brief Implementation of AxisAngle unit tests.
 */

#include "testaxisangle.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

// ========== Field Access Tests ==========

// ========== String Conversion Tests ==========

void TestAxisAngle::TestToString() {
    AxisAngle aa(90.0f, Vector3D(1.0f, 0.0f, 0.0f));

    koilo::UString str = aa.ToString();
    TEST_ASSERT_TRUE(str.Length() > 0);
}

// ========== Axis Vector Tests ==========

// ========== Edge Case Tests ==========

// ========== Test Runner ==========

void TestAxisAngle::TestDefaultConstructor() {
    AxisAngle aa(0.0f, Vector3D(1.0f, 0.0f, 0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, aa.Rotation);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, aa.Axis.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, aa.Axis.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, aa.Axis.Z);
}

void TestAxisAngle::TestEdgeCases() {
    AxisAngle zero(0.0f, Vector3D(1.0f, 0.0f, 0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, zero.Rotation);
    
    AxisAngle full(360.0f, Vector3D(0.0f, 0.0f, 1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 360.0f, full.Rotation);
    
    AxisAngle negative(-45.0f, Vector3D(1.0f, 1.0f, 0.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -45.0f, negative.Rotation);
}

void TestAxisAngle::TestParameterizedConstructor() {
    AxisAngle aa1(45.0f, 1.0f, 0.0f, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 45.0f, aa1.Rotation);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, aa1.Axis.X);
    
    Vector3D axis(0.0f, 1.0f, 0.0f);
    AxisAngle aa2(90.0f, axis);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, aa2.Rotation);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, aa2.Axis.Y);
}

void TestAxisAngle::RunAllTests() {

    RUN_TEST(TestToString);

    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestParameterizedConstructor);
}
